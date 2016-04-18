#include "Mail.h"

Mail::Mail()
{
}


int Mail::sendMail(std::string smtpServerName, std::string sender,
   std::string recipient, std::string subject, std::string msg)
{
   LogContext log;
   log.setContext("Mail");

   if (smtpServerName.empty())
   {
      log.log(Log_DEBUG, "Unable to deliver mail due to missing SMTP server");
      return -1;
   }
   if (sender.empty())
   {
      log.log(Log_DEBUG, "Unable to deliver mail due to missing sender "
         "address");
      return -1;
   }
   if (recipient.empty())
   {
      log.log(Log_DEBUG, "Unable to deliver mail due to missing recipient "
         "address");
      return -1;
   }

   size_t startPoint = sender.find_last_of("@");

   std::string domain = sender.substr(startPoint+1);

   struct addrinfo hint;
   struct addrinfo* addressList;

   memset(&hint, 0, sizeof(struct addrinfo) );
   hint.ai_flags    = AI_CANONNAME;
   hint.ai_family   = PF_INET;
   hint.ai_socktype = SOCK_STREAM;

   int getRes = getaddrinfo(smtpServerName.c_str(), NULL, &hint,
      &addressList);
   if(getRes)
   {
      log.log(Log_DEBUG, "Unable to deliver mail (error while getting address "
         "info for SMTP Server " + smtpServerName + ").");
      return -1;
   }

   int sock = socket(AF_INET, SOCK_STREAM, 0);
   if(sock < 0)
   {
      log.log(Log_SPAM, "Could not create local socket");
      return -1;
   }

   struct sockaddr_in* inetAddr = (struct sockaddr_in*)(addressList->ai_addr);
   inetAddr->sin_port=htons(25);
   inetAddr->sin_family=AF_INET;

   if ( connect(sock, (struct sockaddr*) inetAddr, sizeof(*inetAddr)) < 0 )
   {
      log.log(Log_SPAM, "Unable to deliver mail (Could not connect to SMTP "
         "Server " + smtpServerName + ")");
      return -1;
   }

   int BUF_SIZE = 256;
   char buf[BUF_SIZE];
   std::string ehlo = "EHLO " + domain + "\r\n";
   std::string mailfrom = "MAIL FROM:<" + sender + ">\r\n";
   std::string mailto = "RCPT TO:<" + recipient + ">\r\n";

   struct tm *datetime;

   time_t t;
   time(&t);
   datetime = localtime(&t);

   char timeStr[32];
   sprintf(timeStr, "%s, %.2d %s %.4d %.2d:%.2d:%.2d",
      (intToWeekday(datetime->tm_wday)).c_str(), datetime->tm_mday,
      (intToMonth(datetime->tm_mon)).c_str(), datetime->tm_year+1900,
      datetime->tm_hour, datetime->tm_min, datetime->tm_sec);

   std::string dateStr = "Date: "+std::string(timeStr)+"\r\n";

   std::string data = "DATA\r\n";
   std::string from = "From: <" + sender + ">\r\n";
   std::string to = "To: <" + recipient + ">\r\n";
   std::string subjectStr = "Subject: " + subject + "\r\n";
   std::string message = msg + "\r\n";
   std::string dot = ".\r\n";
   std::string quit = "QUIT\r\n";

   int receivedBytes = 0;

   do
   {
      receivedBytes = recv(sock, buf, BUF_SIZE, 0);
      if(receivedBytes < 0)
      {
         log.log(Log_DEBUG, "Unable to deliver mail (Error in communication "
            "with SMTP Server " + smtpServerName + ")");

         freeaddrinfo(addressList);
         return -1;
      }
   } while (receivedBytes == BUF_SIZE);

   buf[receivedBytes] = '\0';
   send(sock, ehlo.c_str(), strlen(ehlo.c_str()), 0);

   do
   {
      receivedBytes = recv(sock, buf, BUF_SIZE, 0);
      if(receivedBytes < 0)
      {
         log.log(Log_DEBUG,"Unable to deliver mail (Error in communication "
            "with SMTP Server " + smtpServerName+")");
         freeaddrinfo(addressList);
         return -1;
      }
   } while (receivedBytes == BUF_SIZE);
   buf[receivedBytes] = '\0';

   send(sock, mailfrom.c_str(), strlen(mailfrom.c_str()), 0);

   do
   {
      receivedBytes = recv(sock, buf, 256, 0);
      if(receivedBytes < 0)
      {
         log.log(Log_DEBUG, "Unable to deliver mail (Error in communication "
            "with SMTP Server " + smtpServerName + ")");
         freeaddrinfo(addressList);
         return -1;
      }
   } while (receivedBytes == BUF_SIZE);
   buf[receivedBytes] = '\0';

   send(sock, mailto.c_str(), strlen(mailto.c_str()), 0);

   do
   {
      receivedBytes = recv(sock, buf, 256, 0);
      if(receivedBytes < 0)
      {
         log.log(Log_DEBUG, "Unable to deliver mail (Error in communication "
            "with SMTP Server " + smtpServerName + ")");
         freeaddrinfo(addressList);
         return -1;
      }
   } while (receivedBytes == BUF_SIZE);

   buf[receivedBytes] = '\0';

   send(sock, data.c_str(), strlen(data.c_str()), 0);
   do
   {
      receivedBytes = recv(sock, buf, 256, 0);
      if(receivedBytes < 0)
      {
         log.log(Log_DEBUG, "Unable to deliver mail (Error in communication "
            "with SMTP Server " + smtpServerName + ")");
         freeaddrinfo(addressList);
         return -1;
      }
   } while (receivedBytes == BUF_SIZE);

   buf[receivedBytes] = '\0';

   send(sock, from.c_str(), strlen(from.c_str()), 0);
   send(sock, to.c_str(), strlen(to.c_str()), 0);
   send(sock, dateStr.c_str(), strlen(dateStr.c_str()), 0);
   send(sock, subjectStr.c_str(), strlen(subjectStr.c_str()), 0);
   send(sock, message.c_str(), strlen(message.c_str()), 0);
   send(sock, dot.c_str(), strlen(dot.c_str()), 0);
   send(sock, quit.c_str(), strlen(quit.c_str()), 0);

   freeaddrinfo(addressList);
   return 0;
}


