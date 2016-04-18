#ifndef MAIL_H_
#define MAIL_H_

#include <common/Common.h>
#include <common/toolkit/StringTk.h>
#include <common/app/log/LogContext.h>

#include <string.h>
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>

class Mail
{
   public:
      Mail();

      static int sendMail(std::string smtpServerName, std::string sender, std::string recipient,
         std::string subject, std::string msg);

   private:
      static std::string intToWeekday(int intWeekday)
      {
         std::string outWeekday;
         switch (intWeekday)
         {
            case 0: outWeekday="Sun"; break;
            case 1: outWeekday="Mon"; break;
            case 2: outWeekday="Tue"; break;
            case 3: outWeekday="Wed"; break;
            case 4: outWeekday="Thu"; break;
            case 5: outWeekday="Fri"; break;
            case 6: outWeekday="Sat"; break;
         }

         return outWeekday;
      }

      static std::string intToMonth(int intMonth)
      {
         std::string outMonth;
         switch (intMonth)
         {
            case 0 : outMonth="Jan";break;
            case 1 : outMonth="Feb";break;
            case 2 : outMonth="Mar";break;
            case 3 : outMonth="Apr";break;
            case 4 : outMonth="May";break;
            case 5 : outMonth="Jun";break;
            case 6 : outMonth="Jul";break;
            case 7 : outMonth="Aug";break;
            case 8 : outMonth="Sep";break;
            case 9 : outMonth="Oct";break;
            case 10 : outMonth="Nov";break;
            case 11 : outMonth="Dec";break;

         }

         return outMonth;
      }

};

#endif /* MAIL_H_ */
