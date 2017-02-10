#include <program/Program.h>
#include <setup/SetupFunctions.h>

#include "XmlCreator.h"
#include "../../toolkit/StatsAdmon.h"


/*
 * Print the raw information on the HTTP request to stdout;
 * Only for debugging! Will be called from every function, 
 * when DEBUG_MONGOOSE is defined
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::debugMongoose(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
   std::cout << std::endl;
   std::cout << "DEBUG MONGOOSE : " << std::endl;
   std::cout << "request_method : " << request_info->request_method <<
      std::endl;
   std::cout << "uri : " << request_info->uri << std::endl;

   std::string query_str;
   if (request_info->query_string != NULL)
      query_str = request_info->query_string;

   std::cout << "query_string: " << query_str << std::endl;

   if (postBuf != NULL)
      std::cout << "post_data: " << postBuf << std::endl;
}

/*
 * write a XML document to mongoose output stream
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param doc document to send to the remote host
 */
void XmlCreator::writeDoc(struct mg_connection *conn, const struct mg_request_info *request_info,
   TiXmlDocument doc)
{
   Program::getApp()->getLogger()->log(Log_DEBUG, "XmlCreator", "Requested URI: " +
      std::string(request_info->uri) + " from client IP: " + WebTk::getIP(request_info));

   std::string xmlHeader = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
   std::stringstream outStream;
   outStream << xmlHeader;
   outStream << doc;
   std::string outStr = outStream.str();
   int contentLength = outStr.length();

   //send http header
   mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                "Cache: no-cache\r\n"
                "Content-Type: text/xml\r\n"
                "Content-Length: %d\r\n"
                "\r\n",
                contentLength);

   //send data
   mg_write(conn, outStr.c_str(), contentLength);

#ifdef BEEGFS_DEBUG_XML
   Program::getApp()->getLogger()->log(Log_DEBUG, "XmlCreator", "XML content: " + outStr);
#endif
}

/*
 * serve a particular file from the file system via HTTP;
 * is used to provide the Admon GUI via HTTP, but could be used for every
 * other file
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param filename The filename to serve to the remote host
 */
void XmlCreator::serveFile(struct mg_connection *conn, const struct mg_request_info *request_info,
   std::string filename)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, NULL, 0);
#endif

   Program::getApp()->getLogger()->log(Log_DEBUG, "XmlCreator", "Requested URI: " +
      std::string(request_info->uri) + " from client IP: " + WebTk::getIP(request_info));

   std::ifstream ifs(filename.c_str(), std::ifstream::in | std::ifstream::binary);

   if (!ifs.good())
   {
      std::string outStr = std::string("") +
         std::string("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 "
            "Transitional//EN\" ") +
         std::string("\"http://www.w3.org/TR/html4/loose.dtd\">\n") +
         std::string("<html>\n") +
         std::string("    <head>\n") +
         std::string("       <meta http-equiv=\"Content-Type\" "
            "content=\"text/html; charset=UTF-8\">\n") +
         std::string("       <title>BeeGFS Admon GUI download</title>\n") +
         std::string("    </head>\n") +
         std::string("    <body id=\"downloadError\" class=\"lang-en\">\n") +
         std::string("        <p align=\"center\">&nbsp;<br>&nbsp;<br>\n") +
         std::string("        Could not find requested file...\n") +
         std::string("        </p>&nbsp;<br>&nbsp;<br>\n") +
         std::string("    </body>\n") +
         std::string("</html>\n");

      int contentLength = outStr.length();
      mg_printf(conn, "HTTP/1.1 404 Not Found\r\n"
               "Cache: no-cache\r\n"
               "Content-Type: text/html\r\n"
               "Content-Length: %d\r\n"
               "\r\n",
               contentLength);
      mg_write(conn, outStr.c_str(), outStr.length());
   }
   else
   {
      // get length of file:
      ifs.seekg(0, std::ios::end);
      int length = ifs.tellg();
      ifs.seekg(0, std::ios::beg);

      char *output = new char[length];
      ifs.read(output, length);
      ifs.close();

      mg_printf(conn, "HTTP/1.1 200 OK\r\n"
               "Cache: no-cache\r\n"
               "Content-Type: application/java-archive\r\n"
               "Content-Length: %d\r\n"
               "\r\n",
               length);
      mg_write(conn, output, length);
   }
}

/*
 * serve the admon gui over HTTP
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 */
void XmlCreator::admonGui(struct mg_connection *conn,
   const struct mg_request_info *request_info)
{
   XmlCreator::serveFile(conn, request_info, ADMON_GUI_PATH);
}

/*
 * generic error page that can be displayed whenever an unspecific error occurs
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param errorMessage The error message to display
 */
void XmlCreator::sendError(struct mg_connection *conn, const struct mg_request_info *request_info,
   std::string errorMessage)
{
   Program::getApp()->getLogger()->log(Log_ERR, "XmlCreator", "Requested URI: " +
      std::string(request_info->uri) + " from client IP: " + WebTk::getIP(request_info));

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);
   TiXmlElement *errorElement = new TiXmlElement("error");
   rootElement->LinkEndChild(errorElement);

   TiXmlText *errorElementText = new TiXmlText(errorMessage);
   errorElement->LinkEndChild(errorElementText);

   XmlCreator::writeDoc(conn, request_info, doc);
}

/*
 * error page with a given error code and an given error message
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param errorCode The error code to send
 * @param errorMessage The error message to display
 */
void XmlCreator::sendHttpError(struct mg_connection *conn,
   const struct mg_request_info *request_info, const int errorCode, std::string errorMessage)
{
   std::string errorText = "Error";
   if (errorMessage.size() != 0)
      errorText = errorMessage;

   mg_printf(conn, "HTTP/1.1 %d Error\r\n\r\n%s", errorCode, errorText.c_str());
}

/*
 * get a nonce to use for the challenge-response-authentication
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::getNonce(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   int64_t nonce = 0;
   int nonceID = GUITk::createNonce(&nonce);

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   TiXmlElement *idElement = new TiXmlElement("id");
   TiXmlText *idElementText = new TiXmlText(StringTk::intToStr(nonceID));
   idElement->LinkEndChild(idElementText);
   rootElement->LinkEndChild(idElement);

   TiXmlElement *nonceElement = new TiXmlElement("nonce");
   TiXmlText *nonceElementText = new TiXmlText(StringTk::int64ToStr(nonce));
   nonceElement->LinkEndChild(nonceElementText);
   rootElement->LinkEndChild(nonceElement);

   XmlCreator::writeDoc(conn, request_info, doc);
}

/*
 * Does the authentification
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::authInfo(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   std::string nonceID = WebTk::getVar(conn, "nonceID", "", postBuf, postBufLen);
   if (nonceID.empty())
   {
      XmlCreator::sendError(conn, request_info, "nonceID empty");
      return;
   }

   StringList passwords;
   StringList cryptedPasswords;
   Auth::init();
   passwords.push_back(Auth::getPassword("information"));
   passwords.push_back(Auth::getPassword("admin"));

   GUITk::cryptWithNonce(&passwords, &cryptedPasswords,
      StringTk::strToInt(nonceID));

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   StringListIter iter = cryptedPasswords.begin();
   if (iter != cryptedPasswords.end())
   {
      TiXmlElement *element = new TiXmlElement("info");
      TiXmlText *elementText = new TiXmlText(*iter);
      element->LinkEndChild(elementText);
      rootElement->LinkEndChild(element);
      iter++;
   }

   if (iter != cryptedPasswords.end())
   {
      TiXmlElement *element = new TiXmlElement("admin");
      TiXmlText *elementText = new TiXmlText(*iter);
      element->LinkEndChild(elementText);
      rootElement->LinkEndChild(element);
      iter++;
   }

   XmlCreator::writeDoc(conn, request_info, doc);
}

/*
 * Changes the password of the admin or info user
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::changePassword(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   std::string retVal = "false";

   try
   {
      Database *db = Program::getApp()->getDB();
      std::string action = WebTk::getVar(conn, "action", "", postBuf, postBufLen);
      std::string nonceID = WebTk::getVar(conn, "nonceID", "-1", postBuf, postBufLen);

      std::string secret = WebTk::getVar(conn, "secret", "", postBuf, postBufLen);

      TiXmlDocument doc;
      TiXmlElement *rootElement = new TiXmlElement("data");
      doc.LinkEndChild(rootElement);

      TiXmlElement *successElement = new TiXmlElement("success");
      rootElement->LinkEndChild(successElement);

      if (action.compare("enableInfo") == 0)
      {
         Auth::init();
         if (GUITk::checkReply(secret, Auth::getPassword("admin"),
            StringTk::strToInt(nonceID)))
         {
            db->setDisabled("information", false);
            Auth::init();

            retVal = "true";
         }
         else
         {
            retVal = "false";
         }
      }
      else
      if (action.compare("disableInfo") == 0)
      {
         Auth::init();
         if (GUITk::checkReply(secret, Auth::getPassword("admin"), StringTk::strToInt(nonceID)))
         {
            db->setDisabled("information", true);
            Auth::init();

            retVal = "true";
         }
         else
         {
            retVal = "false";
         }
      }
      else
      {
         std::string user = WebTk::getVar(conn, "user", postBuf, postBufLen);
         if ((user.compare("information") == 0) || (user.compare("admin") == 0))
         {
            std::string newPw = WebTk::getVar(conn, "newPw", postBuf, postBufLen);
            if (newPw.compare("") != 0)
            {
               Auth::init();
               if (GUITk::checkReply(secret, Auth::getPassword("admin"),
                  StringTk::strToInt(nonceID)))
               {
                  db->setPassword(user, newPw);
                  Auth::init();

                  retVal = "true";
               }
               else
               {
                  retVal = "false";
               }
            }
            else
            {
               retVal = "false";
            }
         }
         else
         {
            retVal = "false";
         }
      }

      TiXmlText *successElementText = new TiXmlText(retVal);
      successElement->LinkEndChild(successElementText);

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * Checks the architecture and the linux distro of the remote hosts
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::checkDistriArch(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   int checkRes = SetupFunctions::checkDistriArch();

   TiXmlElement *element = new TiXmlElement("finished");
   TiXmlText *elementText = new TiXmlText(checkRes ? "false" : "true");
   element->LinkEndChild(elementText);
   rootElement->LinkEndChild(element);

   XmlCreator::writeDoc(conn, request_info, doc);
}

/*
 * Checks the reachabiliy of a remote host by ssh
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::checkSSH(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   try
   {
      TiXmlDocument doc;
      TiXmlElement *rootElement = new TiXmlElement("data");
      doc.LinkEndChild(rootElement);

      StringList nodeList;
      WebTk::getVarList(conn, "node", &nodeList, postBuf, postBufLen);

      StringList failedNodes;
      SetupFunctions::checkSSH(&nodeList, &failedNodes);
      
      for (StringListIter iter = failedNodes.begin(); iter != failedNodes.end();
         iter++)
      {
         TiXmlElement *nodeElement = new TiXmlElement("node");
         nodeElement->LinkEndChild(new TiXmlText(*iter));
         rootElement->LinkEndChild(nodeElement);
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * stores or gets the basic configuration for the beegfs daemons and the client
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::configBasicConfig(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif
   try
   {
      StringMap paramsMap;
      std::string saveConfig = WebTk::getVar(conn, "saveConfig", "", postBuf, postBufLen);

      TiXmlDocument doc;
      TiXmlElement *rootElement = new TiXmlElement("data");
      doc.LinkEndChild(rootElement);

      if (saveConfig.compare("Save") == 0)
      {
         WebTk::paramsToMap(conn, &paramsMap, "saveConfig", postBuf, postBufLen);
         TiXmlElement *errorElement = new TiXmlElement("errors");
         rootElement->LinkEndChild(errorElement);

         if (SetupFunctions::writeConfigFile(&paramsMap) != 0)
         {
            TiXmlElement *entryElement = new TiXmlElement("entry");
            entryElement->LinkEndChild(new TiXmlText("Could not write the "
               "configuration file."));
            errorElement->LinkEndChild(entryElement);
         }
      }
      else
      {
         SetupFunctions::readConfigFile(&paramsMap);
         for (std::map<std::string, std::string>::iterator
            iter = paramsMap.begin(); iter != paramsMap.end(); iter++)
         {
            std::string key = iter->first;
            std::string value = iter->second;
            if (key.compare("") != 0)
            {
               TiXmlElement *element = new TiXmlElement(key);
               element->LinkEndChild(new TiXmlText(value));
               rootElement->LinkEndChild(element);
            }
         }
      }
      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * stores or gets the Infiniband configuration for the beegfs daemons and the client
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::configIBConfig(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   try
   {
      std::string saveIBConfig = WebTk::getVar(conn, "saveIBConfig", "", postBuf, postBufLen);

      TiXmlDocument doc;
      TiXmlElement *rootElement = new TiXmlElement("data");
      doc.LinkEndChild(rootElement);

      if (saveIBConfig.compare("Save") == 0)
      {
         StringMap paramsMap;
         WebTk::paramsToMap(conn, &paramsMap, "saveIBConfig", postBuf, postBufLen);
         TiXmlElement *errorElement = new TiXmlElement("errors");
         rootElement->LinkEndChild(errorElement);

         if (SetupFunctions::writeIBFile(&paramsMap) != 0)
         {
            TiXmlElement *entryElement = new TiXmlElement("entry");
            entryElement->LinkEndChild(new TiXmlText("Could not write the "
               "configuration file."));
            errorElement->LinkEndChild(entryElement);
         }
      }
      else
      {
         StringMap ibConf;
         StringList hosts;
         if (SetupFunctions::readIBFile(&hosts, &ibConf) == 0)
         {
            for (StringListIter iter = hosts.begin(); iter != hosts.end();
               iter++)
            {
               TiXmlElement *element = new TiXmlElement(*iter);
               rootElement->LinkEndChild(element);

               TiXmlElement *kernelIncPathElem = new TiXmlElement(
                  "kernelIncPath");
               kernelIncPathElem->LinkEndChild(new TiXmlText(
                  ibConf[std::string((*iter) + "_kernelibincpath")]));
               element->LinkEndChild(kernelIncPathElem);
            }
         }
         else
         {
            TiXmlElement *errorElement = new TiXmlElement("error");
            rootElement->LinkEndChild(errorElement);
            TiXmlElement *entryElement = new TiXmlElement("entry");
            entryElement->LinkEndChild(new TiXmlText("Could not read the"
               "configuration file."));
            errorElement->LinkEndChild(entryElement);
         }
      }
      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * stores or gets the role configuration for the beegfs daemons and the clients
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::configRoles(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string saveRoles = WebTk::getVar(conn, "saveRoles", "false", postBuf, postBufLen);
      if (StringTk::strToBool(saveRoles))
      {
         std::string mgmtdNode;
         StringList mgmtdList;
         StringList metaList;
         StringList storageList;
         StringList clientList;
         mgmtdNode = WebTk::getVar(conn, std::string("mgmtd"), postBuf, postBufLen);
         mgmtdList.push_back(mgmtdNode);
         WebTk::getVarList(conn, "meta", &metaList, postBuf, postBufLen);
         WebTk::getVarList(conn, "storage", &storageList, postBuf, postBufLen);
         WebTk::getVarList(conn, "client", &clientList, postBuf, postBufLen);
         TiXmlElement *errorElement = new TiXmlElement("error");
         rootElement->LinkEndChild(errorElement);
         if (SetupFunctions::writeRolesFile(ROLETYPE_Mgmtd, &mgmtdList) != 0)
         {
            TiXmlElement *entryElement = new TiXmlElement("entry");
            entryElement->LinkEndChild(new TiXmlText("Failed to save management nodes."));
            errorElement->LinkEndChild(entryElement);
         }
         if (SetupFunctions::writeRolesFile(ROLETYPE_Meta, &metaList) != 0)
         {
            TiXmlElement *entryElement = new TiXmlElement("entry");
            entryElement->LinkEndChild(new TiXmlText("Failed to save meta nodes."));
            errorElement->LinkEndChild(entryElement);
         }
         if (SetupFunctions::writeRolesFile(ROLETYPE_Storage, &storageList) != 0)
         {
            TiXmlElement *entryElement = new TiXmlElement("entry");
            entryElement->LinkEndChild(new TiXmlText("Failed to save storage nodes."));
            errorElement->LinkEndChild(entryElement);
         }
         if (SetupFunctions::writeRolesFile(ROLETYPE_Client, &clientList) != 0)
         {
            TiXmlElement *entryElement = new TiXmlElement("entry");
            entryElement->LinkEndChild(new TiXmlText("Failed to save client nodes."));
            errorElement->LinkEndChild(entryElement);
         }
      }
      else
      {
         StringList metaList;
         SetupFunctions::readRolesFile(ROLETYPE_Meta, &metaList);
         StringList storageList;
         SetupFunctions::readRolesFile(ROLETYPE_Storage, &storageList);
         StringList clientList;
         SetupFunctions::readRolesFile(ROLETYPE_Client, &clientList);
         StringList mgmtdList;
         SetupFunctions::readRolesFile(ROLETYPE_Mgmtd, &mgmtdList);
         TiXmlElement *metaElement = new TiXmlElement("meta");
         rootElement->LinkEndChild(metaElement);
         for (StringListIter iter = metaList.begin(); iter != metaList.end();
            iter++)
         {
            TiXmlElement *nodeElement = new TiXmlElement("node");
            nodeElement->LinkEndChild(new TiXmlText(*iter));
            metaElement->LinkEndChild(nodeElement);
         }
         TiXmlElement *storageElement = new TiXmlElement("storage");
         rootElement->LinkEndChild(storageElement);
         for (StringListIter iter = storageList.begin();
            iter != storageList.end(); iter++)
         {
            TiXmlElement *nodeElement = new TiXmlElement("node");
            nodeElement->LinkEndChild(new TiXmlText(*iter));
            storageElement->LinkEndChild(nodeElement);
         }
         TiXmlElement *clientElement = new TiXmlElement("client");
         rootElement->LinkEndChild(clientElement);
         for (StringListIter iter = clientList.begin();
            iter != clientList.end(); iter++)
         {
            TiXmlElement *nodeElement = new TiXmlElement("node");
            nodeElement->LinkEndChild(new TiXmlText(*iter));
            clientElement->LinkEndChild(nodeElement);
         }
         TiXmlElement *mgmtdElement = new TiXmlElement("mgmtd");
         rootElement->LinkEndChild(mgmtdElement);
         for (StringListIter iter = mgmtdList.begin(); iter != mgmtdList.end();
            iter++)
         {
            TiXmlElement *nodeElement = new TiXmlElement("node");
            nodeElement->LinkEndChild(new TiXmlText(*iter));
            mgmtdElement->LinkEndChild(nodeElement);
         }
      }
      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * gets all files and directories of a beegfs
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::fileBrowser(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   Display display(Program::getApp()->getMetaNodes(),
      Program::getApp()->getStorageNodes(),
      Program::getApp()->getMgmtNodes());
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string pathStr = WebTk::getVar(conn, "pathStr", "/", postBuf, postBufLen);
      int64_t offset = StringTk::strToInt64(WebTk::getVar(conn, "offset", "0", postBuf,
         postBufLen) );
      int entriesAtOnce = StringTk::strToInt(WebTk::getVar(conn, "entriesAtOnce", "25", postBuf,
         postBufLen) );

      FileEntryList content;
      short retVal = -1;
      uint64_t totalSize = 0;

      retVal = display.listDirFromOffset(pathStr, &offset, &content, &totalSize, entriesAtOnce);
      if (retVal != 0)
      {
         content.clear();
         TiXmlElement *successElement = new TiXmlElement("success");
         successElement->LinkEndChild(new TiXmlText("false"));
         rootElement->LinkEndChild(successElement);
      }
      else
      {
         TiXmlElement *successElement = new TiXmlElement("success");
         successElement->LinkEndChild(new TiXmlText("true"));
         rootElement->LinkEndChild(successElement);

         std::string unit;

         TiXmlElement *generalElement = new TiXmlElement("general");
         rootElement->LinkEndChild(generalElement);

         TiXmlElement *pathElement = new TiXmlElement("path");
         pathElement->LinkEndChild(new TiXmlText(pathStr));
         generalElement->LinkEndChild(pathElement);

         TiXmlElement *offsetElement = new TiXmlElement("offset");
         offsetElement->LinkEndChild(new TiXmlText(
            StringTk::int64ToStr(offset)));
         generalElement->LinkEndChild(offsetElement);

         TiXmlElement *totalFilesElement = new TiXmlElement("totalFiles");
         totalFilesElement->LinkEndChild(new TiXmlText(
            StringTk::uintToStr(content.size())));
         generalElement->LinkEndChild(totalFilesElement);

         TiXmlElement *totalSizeElement = new TiXmlElement("totalSize");
         totalSizeElement->LinkEndChild(new TiXmlText(
            StringTk::int64ToStr(totalSize)));
         generalElement->LinkEndChild(totalSizeElement);

         TiXmlElement *entriesElement = new TiXmlElement("entries");
         rootElement->LinkEndChild(entriesElement);

         for (FileEntryListIter iter = content.begin(); iter != content.end();
            iter++)
         {
            TiXmlElement *entryElement = new TiXmlElement("entry");
            entriesElement->LinkEndChild(entryElement);

            FileEntry entry = *iter;
            entryElement->LinkEndChild(new TiXmlText(entry.name));

            if (S_ISDIR(entry.statData.getMode()))
            {
               entryElement->SetAttribute("isDir", "true");
            }
            else
            {
               entryElement->SetAttribute("isDir", "false");
            }
            entryElement->SetAttribute("filesize", StringTk::int64ToStr(
               entry.statData.getFileSize()));
            entryElement->SetAttribute("mode", display.formatFileMode(
               entry.statData.getMode()));
            entryElement->SetAttribute("ctime", StringTk::int64ToStr(
               entry.statData.getAttribChangeTimeSecs() * 1000));
            entryElement->SetAttribute("mtime", StringTk::int64ToStr(
               entry.statData.getModificationTimeSecs() * 1000));
            entryElement->SetAttribute("atime", StringTk::int64ToStr(
               entry.statData.getLastAccessTimeSecs() * 1000));
            entryElement->SetAttribute("ownerUser", StringTk::uintToStr(
               entry.statData.getUserID()));
            entryElement->SetAttribute("ownerGroup", StringTk::uintToStr(
               entry.statData.getGroupID()));
         }
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * uploads the logfile about the client dependency check
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::getCheckClientDependenciesLog(
   struct mg_connection *conn, const struct mg_request_info *request_info,
   const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string logStr;
      TiXmlElement *successElement = new TiXmlElement("success");
      rootElement->LinkEndChild(successElement);

      if (SetupFunctions::getFileContents(SETUP_CLIENT_WARNING, &logStr) == 0)
      {
         successElement->LinkEndChild(new TiXmlText("true"));
         TiXmlElement *logElement = new TiXmlElement("log");
         logElement->LinkEndChild(new TiXmlText(logStr));
         rootElement->LinkEndChild(logElement);
      }
      else
      {
         successElement->LinkEndChild(new TiXmlText("false"));
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * stores or gets the group configuration for the beegfs daemons and the clients (in the moment
 * not in use)
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::getGroups(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   Database *db = Program::getApp()->getDB();
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string move = WebTk::getVar(conn, "move", "false", postBuf, postBufLen);
      std::string add = WebTk::getVar(conn, "add", "false", postBuf, postBufLen);
      std::string remove = WebTk::getVar(conn, "remove", "false", postBuf, postBufLen);
      std::string nodetype = WebTk::getVar(conn, "nodetype", postBuf, postBufLen);
      std::string source = WebTk::getVar(conn, "source", "", postBuf, postBufLen);
      std::string target = WebTk::getVar(conn, "target", "", postBuf, postBufLen);
      std::string group = WebTk::getVar(conn, "group", "", postBuf, postBufLen);
      UInt16List nodes;
      WebTk::getVarList(conn, "nodes", &nodes, postBuf, postBufLen);

      NodeStoreMetaEx* metaNodes = Program::getApp()->getMetaNodes();
      NodeStoreStorageEx* storageServer = Program::getApp()->getStorageNodes();

      if (StringTk::strToBool(move))
      {
         TiXmlElement *errorElement = new TiXmlElement("errors");
         rootElement->LinkEndChild(errorElement);

         if ((source.compare("") != 0) && (target.compare("") != 0))
         {
            // check if groups exist
            if ((db->getGroupID(source) != -1) && (db->getGroupID(target) != -1))
            {
               for (UInt16ListIter iter = nodes.begin(); iter != nodes.end(); ++iter)
               {
                  if (nodetype.compare("meta") == 0)
                  {
                     Node* tmpNode = metaNodes->referenceNode(*iter);
                     std::string nodeID = tmpNode->getID();
                     metaNodes->releaseNode(&tmpNode);

                     if ((db->delMetaNodeFromGroup(*iter, source) != 0) ||
                        (db->addMetaNodeToGroup(nodeID, *iter, source) != 0))
                     {
                        TiXmlElement *entryElement = new TiXmlElement("entry");
                        entryElement->LinkEndChild(new TiXmlText(
                           "An error occurred on the server side while moving "
                           "the node(s)"));
                        errorElement->LinkEndChild(entryElement);
                     }
                  }
                  else
                  if (nodetype.compare("storage") == 0)
                  {
                     Node* tmpNode = storageServer->referenceNode(*iter);
                     std::string nodeID = tmpNode->getID();
                     storageServer->releaseNode(&tmpNode);

                     if ((db->delStorageNodeFromGroup(*iter, source) != 0)
                        || (db->addStorageNodeToGroup(nodeID, *iter, source) != 0))
                     {
                        TiXmlElement *entryElement = new TiXmlElement("entry");
                        entryElement->LinkEndChild(new TiXmlText(
                           "An error occurred on the server side while moving "
                           "the node(s)"));
                        errorElement->LinkEndChild(entryElement);
                     }
                  }
               }
            }
            else
            {
               TiXmlElement *entryElement = new TiXmlElement("entry");
               entryElement->LinkEndChild(
                  new TiXmlText(
                     "Either the source or the target group does not exist. "
                     "Please reload the groups from server in case someone "
                     "else modified them in the meantime"));
               errorElement->LinkEndChild(entryElement);
            }
         }
      }
      else
      if (StringTk::strToBool(add))
      {
         TiXmlElement *errorElement = new TiXmlElement("errors");
         rootElement->LinkEndChild(errorElement);
         if (group.compare("") != 0)
         {
            if (db->addGroup(group) != 0)
            {
               TiXmlElement *entryElement = new TiXmlElement("entry");
               entryElement->LinkEndChild(new TiXmlText(
                  "An error occurred on the server side while adding the "
                  "group"));
               errorElement->LinkEndChild(entryElement);
            }
         }
      }
      else
      if (StringTk::strToBool(remove))
      {
         TiXmlElement *errorElement = new TiXmlElement("errors");
         rootElement->LinkEndChild(errorElement);

         if (group.compare("") != 0)
         {
            if (db->delGroup(group) != 0)
            {
               TiXmlElement *entryElement = new TiXmlElement("entry");
               entryElement->LinkEndChild(new TiXmlText(
                  "An error occurred on the server side while removing the "
                  "group"));
               errorElement->LinkEndChild(entryElement);
            }
         }
      }
      else
      {
         StringList groups;
         db->getGroups(&groups);
         for (StringListIter iter = groups.begin(); iter != groups.end();
            iter++)
         {
            TiXmlElement *groupElement = new TiXmlElement("group");
            groupElement->LinkEndChild(new TiXmlText(*iter));
            rootElement->LinkEndChild(groupElement);
         }
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * uploads the logfile from a client or beegfs daemon
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::getRemoteLogFile(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string nodeID = WebTk::getVar(conn, "node", "", postBuf, postBufLen);
      std::string service = WebTk::getVar(conn, "service", "", postBuf, postBufLen);
      uint16_t nodeNumID = StringTk::strToUInt(
         WebTk::getVar(conn, "nodeNumID", "", postBuf, postBufLen) );
      uint lines = StringTk::strToUInt(WebTk::getVar(conn, "lines", "0", postBuf, postBufLen) );

      NodeType nodeType;

      if (service.compare("mgmtd") == 0)
         nodeType = NODETYPE_Mgmt;
      else
      if (service.compare("meta") == 0)
         nodeType = NODETYPE_Meta;
      else
      if (service.compare("storage") == 0)
         nodeType = NODETYPE_Storage;
      else
      if (service.compare("admon") == 0)
         nodeType = NODETYPE_Admon;
      else
      if (service.compare("client") == 0)
         nodeType = NODETYPE_Client;
      else
         nodeType = NODETYPE_Invalid;


      std::string logFile;
      if (nodeType == NODETYPE_Admon)
      {
         std::string logFilePath = Program::getApp()->getConfig()->getLogStdFile();
         if(SetupFunctions::getFileContents(logFilePath, &logFile) )
            logFile = "Internal error: Could not load log file.";
      }
      else
      if (nodeType != NODETYPE_Invalid)
      {
         if (SetupFunctions::getLogFile(nodeType, nodeNumID, lines, &logFile, nodeID) )
            logFile = "Internal error: Could not load log file.";
      }
      else // error occurred
         logFile = "Internal error: Could not load log file.";

      TiXmlElement *element = new TiXmlElement("log");
      element->LinkEndChild(new TiXmlText(logFile));
      rootElement->LinkEndChild(element);

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * installs the packages on the remote hosts
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::install(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string package = WebTk::getVar(conn, "package", "", postBuf, postBufLen);
      std::string nonceID = WebTk::getVar(conn, "nonceID", "-1", postBuf, postBufLen);
      std::string secret = WebTk::getVar(conn, "secret", "", postBuf, postBufLen);

      Auth::init();
      if (GUITk::checkReply(secret, Auth::getPassword("admin"),
         StringTk::strToInt(nonceID)))
      {
         TiXmlElement *authenticatedElement = new TiXmlElement("authenticated");
         authenticatedElement->LinkEndChild(new TiXmlText("true"));
         rootElement->LinkEndChild(authenticatedElement);

         TiXmlElement *failedElement = new TiXmlElement("failedHosts");
         rootElement->LinkEndChild(failedElement);

         // write the repository files
         StringList failedRepoHosts;
         StringList failedHosts;

         if (SetupFunctions::createRepos(&failedRepoHosts) )
         {
            for (StringListIter iter = failedRepoHosts.begin();
               iter != failedRepoHosts.end(); iter++)
            {
               TiXmlElement *entryElement = new TiXmlElement("entry");
               entryElement->LinkEndChild(new TiXmlText(*iter));
               failedElement->LinkEndChild(entryElement);
            }
         }
         else
         if (SetupFunctions::installPackage(package, &failedHosts) )
         {
            for (StringListIter iter = failedHosts.begin();
               iter != failedHosts.end(); iter++)
            {
               TiXmlElement *entryElement = new TiXmlElement("entry");
               entryElement->LinkEndChild(new TiXmlText(*iter));
               failedElement->LinkEndChild(entryElement);
            }
         }
         SetupFunctions::updateAdmonConfig();
         XmlCreator::writeDoc(conn, request_info, doc);
      }
      else
      {
         TiXmlElement *authenticatedElement = new TiXmlElement("authenticated");
         authenticatedElement->LinkEndChild(new TiXmlText("false"));
         rootElement->LinkEndChild(authenticatedElement);
         XmlCreator::writeDoc(conn, request_info, doc);
      }
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * gets the installation infos about the remote hosts (role, arch, distro, ...)
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::installInfo(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string check = WebTk::getVar(conn, "check", "false", postBuf, postBufLen);
      std::string package = WebTk::getVar(conn, "package", "", postBuf, postBufLen);
      std::string nonceID = WebTk::getVar(conn, "nonceID", "-1", postBuf, postBufLen);
      std::string secret = WebTk::getVar(conn, "secret", "", postBuf, postBufLen);

      InstallNodeInfoList metaNodes;
      InstallNodeInfoList storageNodes;
      InstallNodeInfoList clientNodes;
      InstallNodeInfoList mgmtNodes;
      SetupFunctions::getInstallNodeInfo(&metaNodes, &storageNodes,
         &clientNodes, &mgmtNodes);
      TiXmlElement *mgmtElement = new TiXmlElement("mgmtd");
      rootElement->LinkEndChild(mgmtElement);

      if (!mgmtNodes.empty())
      {
         InstallNodeInfo info = mgmtNodes.front();
         std::string arch;

         switch (info.arch)
         {
            case ARCH_32:
               arch = "x86 32bit";
               break;
            case ARCH_64:
               arch = "x86 64bit";
               break;
            case ARCH_ppc32:
               arch = "PowerPC 32bit";
               break;
            case ARCH_ppc64:
               arch = "PowerPC 64bit";
               break;
         }
         TiXmlElement *nodeElement = new TiXmlElement("node");
         nodeElement->SetAttribute("arch", arch);
         nodeElement->SetAttribute("dist", info.distributionName);
         nodeElement->LinkEndChild(new TiXmlText(info.name));
         mgmtElement->LinkEndChild(nodeElement);
      }

      TiXmlElement *metaElement = new TiXmlElement("meta");
      rootElement->LinkEndChild(metaElement);
      for (InstallNodeInfoListIter iter = metaNodes.begin();
         iter != metaNodes.end(); iter++)
      {
         InstallNodeInfo info = *iter;
         std::string arch;

         switch (info.arch)
         {
            case ARCH_32:
               arch = "x86 32bit";
               break;
            case ARCH_64:
               arch = "x86 64bit";
               break;
            case ARCH_ppc32:
               arch = "PowerPC 32bit";
               break;
            case ARCH_ppc64:
               arch = "PowerPC 64bit";
               break;
         }

         TiXmlElement *nodeElement = new TiXmlElement("node");
         nodeElement->SetAttribute("arch", arch);
         nodeElement->SetAttribute("dist", info.distributionName);
         nodeElement->LinkEndChild(new TiXmlText(info.name));
         metaElement->LinkEndChild(nodeElement);
      }

      TiXmlElement *storageElement = new TiXmlElement("storage");
      rootElement->LinkEndChild(storageElement);
      for (InstallNodeInfoListIter iter = storageNodes.begin();
         iter != storageNodes.end(); iter++)
      {
         InstallNodeInfo info = *iter;
         std::string arch;

         switch (info.arch)
         {
            case ARCH_32:
               arch = "x86 32bit";
               break;
            case ARCH_64:
               arch = "x86 64bit";
               break;
            case ARCH_ppc32:
               arch = "PowerPC 32bit";
               break;
            case ARCH_ppc64:
               arch = "PowerPC 64bit";
               break;
         }

         TiXmlElement *nodeElement = new TiXmlElement("node");
         nodeElement->SetAttribute("arch", arch);
         nodeElement->SetAttribute("dist", info.distributionName);
         nodeElement->LinkEndChild(new TiXmlText(info.name));
         storageElement->LinkEndChild(nodeElement);
      }

      TiXmlElement *clientElement = new TiXmlElement("client");
      rootElement->LinkEndChild(clientElement);
      for (InstallNodeInfoListIter iter = clientNodes.begin();
         iter != clientNodes.end(); iter++)
      {
         InstallNodeInfo info = *iter;
         std::string arch;

         switch (info.arch)
         {
            case ARCH_32:
               arch = "x86 32bit";
               break;
            case ARCH_64:
               arch = "x86 64bit";
               break;
            case ARCH_ppc32:
               arch = "PowerPC 32bit";
               break;
            case ARCH_ppc64:
               arch = "PowerPC 64bit";
               break;
         }

         TiXmlElement *nodeElement = new TiXmlElement("node");
         nodeElement->SetAttribute("arch", arch);
         nodeElement->SetAttribute("dist", info.distributionName);
         nodeElement->LinkEndChild(new TiXmlText(info.name));
         clientElement->LinkEndChild(nodeElement);
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * removes the packages on the remote hosts
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::uninstall(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string package = WebTk::getVar(conn, "package", "", postBuf, postBufLen);
      std::string nonceID = WebTk::getVar(conn, "nonceID", "-1", postBuf, postBufLen);
      std::string secret = WebTk::getVar(conn, "secret", "", postBuf, postBufLen);

      Auth::init();
      if (GUITk::checkReply(secret, Auth::getPassword("admin"),
         StringTk::strToInt(nonceID)))
      {
         TiXmlElement *authenticatedElement = new TiXmlElement("authenticated");
         authenticatedElement->LinkEndChild(new TiXmlText("true"));
         rootElement->LinkEndChild(authenticatedElement);

         StringList failedHosts;
         TiXmlElement *failedElement = new TiXmlElement("failedHosts");
         rootElement->LinkEndChild(failedElement);
         if (SetupFunctions::uninstallPackage(package, &failedHosts) != 0)
         {
            for (StringListIter iter = failedHosts.begin();
               iter != failedHosts.end(); iter++)
            {
               TiXmlElement *entryElement = new TiXmlElement("entry");
               entryElement->LinkEndChild(new TiXmlText(*iter));
               failedElement->LinkEndChild(entryElement);
            }
         }
      }
      else
      {
         TiXmlElement *authenticatedElement = new TiXmlElement("authenticated");
         authenticatedElement->LinkEndChild(new TiXmlText("false"));
         rootElement->LinkEndChild(authenticatedElement);
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * gets all known problems of the beegfs
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::knownProblems(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   Display display(Program::getApp()->getMetaNodes(),
      Program::getApp()->getStorageNodes(),
      Program::getApp()->getMgmtNodes());
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      NodeStoreMetaEx *metaNodeStore = Program::getApp()->getMetaNodes();
      NodeList metaNodesList;
      metaNodeStore->referenceAllNodes(&metaNodesList);
      StringList deadMetaNodes;
      std::string info;
      for (NodeListIter iter = metaNodesList.begin();
         iter != metaNodesList.end(); iter++)
      {
         if (!(display.statusMeta((*iter)->getNumID(), &info)))
         {
            deadMetaNodes.push_back((*iter)->getID());
         }
      }

      metaNodeStore->releaseAllNodes(&metaNodesList);

      NodeStoreStorageEx *storageNodeStore = Program::getApp()->getStorageNodes();
      NodeList storageNodesList;
      storageNodeStore->referenceAllNodes(&storageNodesList);
      StringList deadStorageNodes;
      for (NodeListIter iter = storageNodesList.begin();
         iter != storageNodesList.end(); iter++)
      {
         if (!(display.statusStorage((*iter)->getNumID(), &info)))
         {
            deadStorageNodes.push_back((*iter)->getID());
         }
      }

      storageNodeStore->releaseAllNodes(&storageNodesList);

      TiXmlElement *metaElement = new TiXmlElement("deadMetaNodes");
      rootElement->LinkEndChild(metaElement);

      if (!deadMetaNodes.empty())
      {
         for (StringListIter iter = deadMetaNodes.begin();
            iter != deadMetaNodes.end(); iter++)
         {
            TiXmlElement *nodeElement = new TiXmlElement("node");
            nodeElement->LinkEndChild(new TiXmlText(*iter));
            metaElement->LinkEndChild(nodeElement);
         }
      }

      TiXmlElement *storageElement = new TiXmlElement("deadStorageNodes");
      rootElement->LinkEndChild(storageElement);

      if (!deadStorageNodes.empty())
      {
         for (StringListIter iter = deadStorageNodes.begin();
            iter != deadStorageNodes.end(); iter++)
         {
            TiXmlElement *nodeElement = new TiXmlElement("node");
            nodeElement->LinkEndChild(new TiXmlText(*iter));
            storageElement->LinkEndChild(nodeElement);
         }
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * uploads the setup logfile
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::getLocalLogFile(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string logStr;
      TiXmlElement *successElement = new TiXmlElement("success");
      rootElement->LinkEndChild(successElement);

      if (SetupFunctions::getFileContents(SETUP_LOG_PATH, &logStr) == 0)
      {
         successElement->LinkEndChild(new TiXmlText("true"));
         TiXmlElement *logElement = new TiXmlElement("log");
         logElement->LinkEndChild(new TiXmlText(logStr));
         rootElement->LinkEndChild(logElement);
      }
      else
      {
         successElement->LinkEndChild(new TiXmlText("false"));
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * gets or stores the eMail notification settings
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::mailNotification(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   Database *db = Program::getApp()->getDB();
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string change = WebTk::getVar(conn, "change", "false", postBuf, postBufLen);
      std::string mailEnabled = WebTk::getVar(conn, "mailEnabled", "false", postBuf, postBufLen);
      std::string sendType = WebTk::getVar(conn, "sendType", "", postBuf, postBufLen);
      std::string sendMailPath = WebTk::getVar(conn, "sendmailPath", "", postBuf, postBufLen);
      std::string smtpServer = WebTk::getVar(conn, "smtpServer", "", postBuf, postBufLen);
      std::string sender = WebTk::getVar(conn, "sender", "", postBuf, postBufLen);
      std::string recipient = WebTk::getVar(conn, "recipient", "", postBuf, postBufLen);
      int mailMinDownTimeSec = StringTk::strToInt(
         WebTk::getVar(conn, "mailMinDownTimeSec", "0", postBuf, postBufLen) );
      int mailResendMailTimeMin = StringTk::strToInt(
         WebTk::getVar(conn, "mailResendMailTimeMin", "0", postBuf, postBufLen) );
      std::string nonceID = WebTk::getVar(conn, "nonceID", "-1", postBuf, postBufLen);
      std::string secret = WebTk::getVar(conn, "secret", "", postBuf, postBufLen);

      RuntimeConfig *runtimeCfg = Program::getApp()->getRuntimeConfig();
      Config *cfg = Program::getApp()->getConfig();

      if (StringTk::strToBool(change))
      {
         TiXmlElement *errorElement = new TiXmlElement("errors");
         rootElement->LinkEndChild(errorElement);

         bool enableMail = StringTk::strToBool(mailEnabled);
         Auth::init();

         if (!GUITk::checkReply(secret, Auth::getPassword("admin"),
            StringTk::strToInt(nonceID)))
         {
            TiXmlElement *entryElement = new TiXmlElement("entry");
            entryElement->LinkEndChild(new TiXmlText(
               "The system could not authenticate you as administrative user"));
            errorElement->LinkEndChild(entryElement);
         }
         else if(enableMail && ( sender.empty() || recipient.empty() ||
            ( (StringTk::strToInt(sendType) == SmtpSendType_SOCKET) && smtpServer.empty() ) ||
            ( (StringTk::strToInt(sendType) == SmtpSendType_SENDMAIL) && sendMailPath.empty() ) ) )
         {
            bool notFirstEmptyValue = false;

            std::string errorMessage = "Could not write new config. Please be aware that if you"
               "activate eMail support, you MUST specify a ";

            if(sender.empty() )
            {
               errorMessage.append("sender");
               notFirstEmptyValue = true;
            }

            if(recipient.empty() )
            {
               if(notFirstEmptyValue)
               {
                  errorMessage.append(", recipient");
               }
               else
               {
                  errorMessage.append("recipient");
                  notFirstEmptyValue = true;
               }

            }

            if( (StringTk::strToInt(sendType) == SmtpSendType_SOCKET) && smtpServer.empty() )
            {
               if(notFirstEmptyValue)
               {
                  errorMessage.append(", SMTP Server");
               }
               else
               {
                  errorMessage.append("SMTP Server");
                  notFirstEmptyValue = true;
               }
            }

            if( (StringTk::strToInt(sendType) == SmtpSendType_SENDMAIL) && sendMailPath.empty() )
            {
               if(notFirstEmptyValue)
               {
                  errorMessage.append(", path to sendmail");
               }
               else
               {
                  errorMessage.append("path to sendmail");
               }
            }
            errorMessage.append(".");

            TiXmlElement *entryElement = new TiXmlElement("entry");
            entryElement->LinkEndChild(new TiXmlText(errorMessage) );
            errorElement->LinkEndChild(entryElement);
         }
         else
         {
            runtimeCfg->setMailEnabled(enableMail);
            runtimeCfg->setMailSmtpSendType( (SmtpSendType)StringTk::strToInt(sendType) );
            runtimeCfg->setMailSendmailPath(sendMailPath);
            runtimeCfg->setMailSmtpServer(smtpServer);
            runtimeCfg->setMailSender(sender);
            runtimeCfg->setMailRecipient(recipient);
            runtimeCfg->setMailMinDownTimeSec(mailMinDownTimeSec);
            runtimeCfg->setMailResendMailTimeMin(mailResendMailTimeMin);

            if (db->writeRuntimeConfig() != 0)
            {
               TiXmlElement *entryElement = new TiXmlElement("entry");
               entryElement->LinkEndChild(new TiXmlText(
                  "Could not write new config. Please be aware that if you activate eMail support, "
                  "you MUST specify a sender and a recipient.") );
               errorElement->LinkEndChild(entryElement);
            }
         }
      }
      else
      {
         TiXmlElement *enabledElement = new TiXmlElement("enabled");
         if (runtimeCfg->getMailEnabled())
         {
            enabledElement->LinkEndChild(new TiXmlText("true"));
         }
         else
         {
            enabledElement->LinkEndChild(new TiXmlText("false"));
         }
         rootElement->LinkEndChild(enabledElement);

         TiXmlElement *overrideActiveElement = new TiXmlElement("overrideActive");
         if (cfg->isMailConfigOverrideActive())
         {
            overrideActiveElement->LinkEndChild(new TiXmlText("true"));
         }
         else
         {
            overrideActiveElement->LinkEndChild(new TiXmlText("false"));
         }
         rootElement->LinkEndChild(overrideActiveElement);

         TiXmlElement *sendTypeElement = new TiXmlElement("sendType");
         sendTypeElement->LinkEndChild(new TiXmlText(
            StringTk::intToStr(runtimeCfg->getMailSmtpSendType() ) ) );
         rootElement->LinkEndChild(sendTypeElement);

         TiXmlElement *sendmailPathElement = new TiXmlElement("sendmailPath");
         sendmailPathElement->LinkEndChild(new TiXmlText(runtimeCfg->getMailSendmailPath() ) );
         rootElement->LinkEndChild(sendmailPathElement);

         TiXmlElement *smtpElement = new TiXmlElement("smtpServer");
         smtpElement->LinkEndChild(new TiXmlText(runtimeCfg->getMailSmtpServer() ) );
         rootElement->LinkEndChild(smtpElement);

         TiXmlElement *senderElement = new TiXmlElement("sender");
         senderElement->LinkEndChild(new TiXmlText(runtimeCfg->getMailSender() ) );
         rootElement->LinkEndChild(senderElement);

         TiXmlElement *recipientElement = new TiXmlElement("recipient");
         recipientElement->LinkEndChild(new TiXmlText(runtimeCfg->getMailRecipient() ) );
         rootElement->LinkEndChild(recipientElement);

         TiXmlElement *delayElement = new TiXmlElement("delay");
         delayElement->LinkEndChild(new TiXmlText(StringTk::intToStr(
            runtimeCfg->getMailMinDownTimeSec())));
         rootElement->LinkEndChild(delayElement);

         TiXmlElement *resendElement = new TiXmlElement("resendTime");
         resendElement->LinkEndChild(new TiXmlText(StringTk::intToStr(
            runtimeCfg->getMailResendMailTimeMin() ) ) );
         rootElement->LinkEndChild(resendElement);
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * gets the details of one metadata node
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::metaNode(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   Display display(Program::getApp()->getMetaNodes(),
      Program::getApp()->getStorageNodes(),
      Program::getApp()->getMgmtNodes());

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      uint timeSpanRequests = StringTk::strToUInt(WebTk::getVar(
         conn, "timeSpanRequests", "10", postBuf, postBufLen).c_str() );

      std::string node = WebTk::getVar(conn, "node", postBuf, postBufLen);
      uint16_t nodeNumID = StringTk::strToUInt(WebTk::getVar(conn, "nodeNumID", postBuf,
         postBufLen) );
      TiXmlElement *generalElement = new TiXmlElement("general");
      rootElement->LinkEndChild(generalElement);

      TiXmlElement *nodeIDElement = new TiXmlElement("nodeID");
      nodeIDElement->LinkEndChild(new TiXmlText(node));
      generalElement->LinkEndChild(nodeIDElement);
      TiXmlElement *nodeNumIDElement = new TiXmlElement("nodeNumID");
      nodeNumIDElement->LinkEndChild(new TiXmlText(StringTk::uintToStr(nodeNumID)));
      generalElement->LinkEndChild(nodeNumIDElement);

      std::string outInfo;
      if (display.statusMeta(nodeNumID, &outInfo))
      {
         TiXmlElement *statusElement = new TiXmlElement("status");
         statusElement->LinkEndChild(new TiXmlText("true"));
         generalElement->LinkEndChild(statusElement);
      }
      else
      {
         TiXmlElement *statusElement = new TiXmlElement("status");
         statusElement->LinkEndChild(new TiXmlText("false"));
         generalElement->LinkEndChild(statusElement);
      }

      GeneralNodeInfo info;

      if (display.getGeneralMetaNodeInfo(nodeNumID, &info))
      {
         TiXmlElement *cpuModelElement = new TiXmlElement("cpuModel");
         cpuModelElement->LinkEndChild(new TiXmlText(info.cpuName));
         generalElement->LinkEndChild(cpuModelElement);

         TiXmlElement *cpuCountElement = new TiXmlElement("cpuCount");
         cpuCountElement->LinkEndChild(new TiXmlText(
            StringTk::intToStr(info.cpuCount)));
         generalElement->LinkEndChild(cpuCountElement);

         char cpuSpeedStr[32];
         sprintf(cpuSpeedStr, "%0.1lf", (double) (info.cpuSpeed) / 1000);
         TiXmlElement *cpuSpeedElement = new TiXmlElement("cpuSpeed");
         cpuSpeedElement->LinkEndChild(
            new TiXmlText(std::string(cpuSpeedStr) + std::string(" GHz")));
         generalElement->LinkEndChild(cpuSpeedElement);

         char ramStr[32];
         sprintf(ramStr, "%0.1lf", (double) (info.memTotal) / 1000);
         TiXmlElement *ramSizeElement = new TiXmlElement("ramSize");
         ramSizeElement->LinkEndChild(new TiXmlText(std::string(ramStr) +
            std::string(" GB")));
         generalElement->LinkEndChild(ramSizeElement);
      }
      else
      {
         TiXmlElement *cpuModelElement = new TiXmlElement("cpuModel");
         cpuModelElement->LinkEndChild(new TiXmlText(""));
         generalElement->LinkEndChild(cpuModelElement);

         TiXmlElement *cpuCountElement = new TiXmlElement("cpuCount");
         cpuCountElement->LinkEndChild(new TiXmlText(""));
         generalElement->LinkEndChild(cpuCountElement);

         TiXmlElement *cpuSpeedElement = new TiXmlElement("cpuSpeed");
         cpuSpeedElement->LinkEndChild(new TiXmlText(""));
         generalElement->LinkEndChild(cpuSpeedElement);

         TiXmlElement *ramSizeElement = new TiXmlElement("ramSize");
         ramSizeElement->LinkEndChild(new TiXmlText(""));
         generalElement->LinkEndChild(ramSizeElement);
      }

      TiXmlElement *rootNodeElement = new TiXmlElement("rootNode");
      rootNodeElement->LinkEndChild(new TiXmlText(display.isRootMetaNode(nodeNumID)));
      generalElement->LinkEndChild(rootNodeElement);

      TiXmlElement *lastMessageElement = new TiXmlElement("lastMessage");
      lastMessageElement->LinkEndChild(new TiXmlText(display.
         timeSinceLastMessageMeta(nodeNumID)));
      generalElement->LinkEndChild(lastMessageElement);

      TiXmlElement *openSessionsElement = new TiXmlElement("openSessions");
      openSessionsElement->LinkEndChild(new TiXmlText(StringTk::uintToStr(
         display.sessionCountMeta(nodeNumID))));
      generalElement->LinkEndChild(openSessionsElement);

      StringList times;
      StringList workRequests;
      StringList queuedRequests;
      display.metaDataRequests(nodeNumID, timeSpanRequests, &times, &queuedRequests, &workRequests);

      TiXmlElement *workRequestsElement = new TiXmlElement("workRequests");
      rootElement->LinkEndChild(workRequestsElement);
      TiXmlElement *queuedRequestsElement = new TiXmlElement("queuedRequests");
      rootElement->LinkEndChild(queuedRequestsElement);

      StringListIter timesIter = times.begin();
      StringListIter workRequestsIter = workRequests.begin();
      StringListIter queuedRequestsIter = queuedRequests.begin();

      while (timesIter != times.end())
      {
         if (workRequestsIter != workRequests.end())
         {
            TiXmlElement *workRequestsValueElement =new TiXmlElement("value");
            workRequestsValueElement->SetAttribute("time", *timesIter);
            workRequestsValueElement->LinkEndChild(new TiXmlText(*workRequestsIter));
            workRequestsElement->LinkEndChild(workRequestsValueElement);

            workRequestsIter++;
         }

         if (queuedRequestsIter != queuedRequests.end())
         {
            TiXmlElement *queuedRequestsValueElement = new TiXmlElement("value");
            queuedRequestsValueElement->SetAttribute("time", *timesIter);
            queuedRequestsValueElement->LinkEndChild(new TiXmlText(*queuedRequestsIter));
            queuedRequestsElement->LinkEndChild(queuedRequestsValueElement);
            queuedRequestsIter++;
         }
         timesIter++;
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * gets the overview about all metadata nodes
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::metaNodesOverview(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   Database *db = Program::getApp()->getDB();
   Display display(Program::getApp()->getMetaNodes(),
      Program::getApp()->getStorageNodes(),
      Program::getApp()->getMgmtNodes());
      
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      uint timeSpanRequests = StringTk::strToUInt(
         WebTk::getVar(conn, "timeSpanRequests", "10", postBuf, postBufLen).c_str());

      std::string group = WebTk::getVar(conn, "group", "", postBuf, postBufLen);

      UInt16List nodeNumIDs;
      StringList nodes;

      if (group.compare("") == 0)
      {
         Program::getApp()->getMetaNodeNumIDs(&nodeNumIDs);
         Program::getApp()->getMetaNodesAsStringList(&nodes);
      }
      else
      {
         db->getMetaNodesInGroup(group, &nodeNumIDs);
         db->getMetaNodesInGroup(group, &nodes);
      }

      TiXmlElement *generalElement = new TiXmlElement("general");
      rootElement->LinkEndChild(generalElement);

      TiXmlElement *nodeCountElement = new TiXmlElement("nodeCount");
      nodeCountElement->LinkEndChild(new TiXmlText(
         StringTk::intToStr(display.countMetaNodes())));
      generalElement->LinkEndChild(nodeCountElement);

      TiXmlElement *rootNodeElement = new TiXmlElement("rootNode");
      rootNodeElement->LinkEndChild(new TiXmlText(display.getRootMetaNodeAsTypedNodeID()));
      generalElement->LinkEndChild(rootNodeElement);

      TiXmlElement *statusElement = new TiXmlElement("status");
      rootElement->LinkEndChild(statusElement);

      UInt16ListIter nodeNumIDIter = nodeNumIDs.begin();
      for (StringListIter nodeIDIter = nodes.begin(); nodeIDIter != nodes.end(); nodeIDIter++,
            nodeNumIDIter++)
      {
         std::string outInfo;
         std::string nodeID = *nodeIDIter;
         uint16_t nodeNumID = *nodeNumIDIter;

         TiXmlElement *valueElement = new TiXmlElement("value");
         statusElement->LinkEndChild(valueElement);
         valueElement->SetAttribute("node", nodeID);
         valueElement->SetAttribute("nodeNumID", nodeNumID);

         if (display.statusMeta(nodeNumID, &outInfo))
            valueElement->LinkEndChild(new TiXmlText("true"));
         else
            valueElement->LinkEndChild(new TiXmlText("false"));
      }

      StringList times;
      StringList workRequests;
      StringList queuedRequests;
      display.metaDataRequestsSum(timeSpanRequests, &times, &queuedRequests, &workRequests);

      TiXmlElement *workRequestsElement = new TiXmlElement("workRequests");
      rootElement->LinkEndChild(workRequestsElement);
      TiXmlElement *queuedRequestsElement = new TiXmlElement("queuedRequests");
      rootElement->LinkEndChild(queuedRequestsElement);

      StringListIter timesIter = times.begin();
      StringListIter workRequestsIter = workRequests.begin();
      StringListIter queuedRequestsIter = queuedRequests.begin();

      while (timesIter != times.end())
      {
         if (workRequestsIter != workRequests.end())
         {
            TiXmlElement *workRequestsValueElement = new TiXmlElement("value");
            workRequestsValueElement->SetAttribute("time", *timesIter);
            workRequestsValueElement->LinkEndChild(
               new TiXmlText(*workRequestsIter));
            workRequestsElement->LinkEndChild(workRequestsValueElement);

            workRequestsIter++;
         }

         if (queuedRequestsIter != queuedRequests.end())
         {
            TiXmlElement *queuedRequestsValueElement =
               new TiXmlElement("value");
            queuedRequestsValueElement->SetAttribute("time", *timesIter);
            queuedRequestsValueElement->LinkEndChild(
               new TiXmlText(*queuedRequestsIter));
            queuedRequestsElement->LinkEndChild(queuedRequestsValueElement);

            queuedRequestsIter++;
         }

         timesIter++;
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * gets a node list with the group informations
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::nodeList(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   App* app = Program::getApp();
   Database *db = app->getDB();
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      bool clients = StringTk::strToBool(
         WebTk::getVar(conn, "clients", "false", postBuf, postBufLen));
      bool admon = StringTk::strToBool(
         WebTk::getVar(conn, "admon", "false", postBuf, postBufLen));


      TiXmlElement *mgmtdElement = new TiXmlElement("mgmtd");
      rootElement->LinkEndChild(mgmtdElement);

      NodeStoreMgmtEx *mgmtdNodeStore = app->getMgmtNodes();
      Node *node = mgmtdNodeStore->referenceFirstNode();
      while (node != NULL)
      {
         std::string groupName = "Default";
         std::string nodeID = node->getID();
         uint16_t nodeNumID = node->getNumID();

         TiXmlElement *nodeElement = new TiXmlElement("node");
         nodeElement->SetAttribute("group", groupName);
         nodeElement->SetAttribute("nodeNumID", StringTk::uintToStr(nodeNumID));
         nodeElement->LinkEndChild(new TiXmlText(nodeID));
         mgmtdElement->LinkEndChild(nodeElement);

         node = mgmtdNodeStore->referenceNextNodeAndReleaseOld(node);
      }

      TiXmlElement *metaElement = new TiXmlElement("meta");
      rootElement->LinkEndChild(metaElement);

      NodeStoreMetaEx *metaNodeStore = app->getMetaNodes();
      node = metaNodeStore->referenceFirstNode();
      while (node != NULL)
      {
         std::string groupName = "Default";
         std::string nodeID = node->getID();
         uint64_t nodeNumID = node->getNumID();

         int groupID = db->metaNodeGetGroup(nodeNumID);
         if (groupID >= 0)
            groupName = db->getGroupName(groupID);

         TiXmlElement *nodeElement = new TiXmlElement("node");
         nodeElement->SetAttribute("group", groupName);
         nodeElement->SetAttribute("nodeNumID", StringTk::uintToStr(nodeNumID));
         nodeElement->LinkEndChild(new TiXmlText(nodeID));
         metaElement->LinkEndChild(nodeElement);

         node = metaNodeStore->referenceNextNodeAndReleaseOld(node);
      }

      TiXmlElement *storageElement = new TiXmlElement("storage");
      rootElement->LinkEndChild(storageElement);
      NodeStoreStorageEx *storageNodeStore = app->getStorageNodes();
      node = storageNodeStore->referenceFirstNode();
      while (node != NULL)
      {
         std::string groupName = "Default";
         std::string nodeID = node->getID();
         uint64_t nodeNumID = node->getNumID();

         int groupID = db->storageNodeGetGroup(nodeNumID);
         if (groupID >= 0)
            groupName = db->getGroupName(groupID);

         TiXmlElement *nodeElement = new TiXmlElement("node");
         nodeElement->SetAttribute("group", groupName);
         nodeElement->SetAttribute("nodeNumID", StringTk::uintToStr(nodeNumID));
         nodeElement->LinkEndChild(new TiXmlText(nodeID));
         storageElement->LinkEndChild(nodeElement);

         node = storageNodeStore->referenceNextNodeAndReleaseOld(node);
      }

      if (clients)
      {
         TiXmlElement *clientElement = new TiXmlElement("client");
         rootElement->LinkEndChild(clientElement);

         NodeStoreClients *clientNodeStore = app->getClientNodes();

         Node *node = clientNodeStore->referenceFirstNode();
         while (node != NULL)
         {
            std::string groupName = "Default";
            std::string nodeID = node->getID();
            uint64_t nodeNumID = node->getNumID();

            TiXmlElement *nodeElement = new TiXmlElement("node");
            nodeElement->SetAttribute("group", groupName);
            nodeElement->SetAttribute("nodeNumID", StringTk::uintToStr(nodeNumID));
            nodeElement->LinkEndChild(new TiXmlText(nodeID));
            clientElement->LinkEndChild(nodeElement);

            clientNodeStore->releaseNode(&node);
            node = clientNodeStore->referenceNextNode(nodeID);
         }
      }

      if(admon)
      {
         TiXmlElement *admonElement = new TiXmlElement("admon");
         rootElement->LinkEndChild(admonElement);

         Node* localNode = app->getLocalNode();
         std::string groupName = "Default";
         std::string nodeID = localNode->getID();
         uint64_t nodeNumID = localNode->getNumID();

         TiXmlElement *nodeElement = new TiXmlElement("node");
         nodeElement->SetAttribute("group", groupName);
         nodeElement->SetAttribute("nodeNumID", StringTk::uintToStr(nodeNumID));
         nodeElement->LinkEndChild(new TiXmlText(nodeID));
         admonElement->LinkEndChild(nodeElement);
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * checks if the passwordless access for the info users is enabled
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::preAuthInfo(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      TiXmlElement *infoDisabledElement = new TiXmlElement("infoDisabled");
      rootElement->LinkEndChild(infoDisabledElement);

      if (Auth::getInformationDisabled())
         infoDisabledElement->LinkEndChild(new TiXmlText("true"));
      else
         infoDisabledElement->LinkEndChild(new TiXmlText("false"));

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * gets or stores the script settings
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::scriptSettings(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   Database *db = Program::getApp()->getDB();
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string change = WebTk::getVar(conn, "change", "false", postBuf, postBufLen);
      std::string scriptPath = WebTk::getVar(conn, "scriptPath", "", postBuf, postBufLen);
      std::string nonceID = WebTk::getVar(conn, "nonceID", "-1", postBuf, postBufLen);
      std::string secret = WebTk::getVar(conn, "secret", "", postBuf, postBufLen);

      RuntimeConfig *cfg = Program::getApp()->getRuntimeConfig();
      if (StringTk::strToBool(change))
      {
         TiXmlElement *errorElement = new TiXmlElement("errors");
         rootElement->LinkEndChild(errorElement);

         Auth::init();

         if (!GUITk::checkReply(secret, Auth::getPassword("admin"),
            StringTk::strToInt(nonceID)))
         {
            TiXmlElement *entryElement = new TiXmlElement("entry");
            entryElement->LinkEndChild(new TiXmlText(
               "The system could not authenticate you as administrative user"));
            errorElement->LinkEndChild(entryElement);
         }
         else
         {
            if ((!scriptPath.empty()) &&
               (access(scriptPath.c_str(), F_OK) != 0))
            {
               // path does not exist or is not a file
               TiXmlElement *entryElement = new TiXmlElement("entry");
               entryElement->LinkEndChild(new TiXmlText(
                  "New config not written (path does not exist or is not "
                  "a file)."));
               errorElement->LinkEndChild(entryElement);
            }
            else
            if ((!scriptPath.empty()) && (access(scriptPath.c_str(),
               X_OK) != 0))
            {
               // script not executable
               TiXmlElement *entryElement = new TiXmlElement("entry");
               entryElement->LinkEndChild(new TiXmlText(
                  "New config not written (script is not executable)."));
               errorElement->LinkEndChild(entryElement);
            }
            else
            {
               cfg->setScriptPath(scriptPath);

               if (db->writeRuntimeConfig() != 0)
               {
                  TiXmlElement *entryElement = new TiXmlElement("entry");
                  entryElement->LinkEndChild(new TiXmlText(
                     "Could not write new config (database error)."));
                  errorElement->LinkEndChild(entryElement);
               }
            }

         }
      }
      else
      {
         TiXmlElement *scriptPathElement = new TiXmlElement("scriptPath");
         scriptPathElement->LinkEndChild(new TiXmlText(cfg->getScriptPath()));
         rootElement->LinkEndChild(scriptPathElement);
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * creates a new setup logfile
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::createNewSetupLogfile(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string nonceID = WebTk::getVar(conn, "nonceID", "-1", postBuf, postBufLen);
      std::string secret = WebTk::getVar(conn, "secret", "", postBuf, postBufLen);
      std::string type = WebTk::getVar(conn, "type", "", postBuf, postBufLen);

      Auth::init();

      if (GUITk::checkReply(secret, Auth::getPassword("admin"),
         StringTk::strToInt(nonceID)))
      {
         TiXmlElement *authenticatedElement = new TiXmlElement("authenticated");
         authenticatedElement->LinkEndChild(new TiXmlText("true"));
         rootElement->LinkEndChild(authenticatedElement);
         XmlCreator::writeDoc(conn, request_info, doc);
         SetupFunctions::createNewSetupLogfile(type);
      }
      else
      {
         TiXmlElement *authenticatedElement = new TiXmlElement("authenticated");
         authenticatedElement->LinkEndChild(new TiXmlText("false"));
         rootElement->LinkEndChild(authenticatedElement);
         XmlCreator::writeDoc(conn, request_info, doc);
      }
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * starts or stops beegfs daemons or clients
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::startStop(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string nonceID = WebTk::getVar(conn, "nonceID", "-1", postBuf, postBufLen);
      std::string secret = WebTk::getVar(conn, "secret", "", postBuf, postBufLen);
      std::string start = WebTk::getVar(conn, "start", "false", postBuf, postBufLen);
      std::string stop = WebTk::getVar(conn, "stop", "false", postBuf, postBufLen);
      std::string startAll = WebTk::getVar(conn, "startAll", "false", postBuf, postBufLen);
      std::string stopAll = WebTk::getVar(conn, "stopAll", "false", postBuf, postBufLen);
      std::string restartAdmon = WebTk::getVar(conn, "restartAdmon", "false", postBuf, postBufLen);
      std::string host = WebTk::getVar(conn, "host", "", postBuf, postBufLen);
      std::string service = WebTk::getVar(conn, "service", "", postBuf, postBufLen);

      TiXmlElement *authElement = new TiXmlElement("authenticated");
      rootElement->LinkEndChild(authElement);

      if (StringTk::strToBool(restartAdmon))
      {
         Auth::init();

         if (GUITk::checkReply(secret, Auth::getPassword("admin"),
            StringTk::strToInt(nonceID)))
         {
            authElement->LinkEndChild(new TiXmlText("true"));
            SetupFunctions::restartAdmon();
         }
         else
         {
            authElement->LinkEndChild(new TiXmlText("false"));
         }
      }
      else
      if (StringTk::strToBool(startAll))
      {
         Auth::init();

         if (GUITk::checkReply(secret, Auth::getPassword("admin"),
            StringTk::strToInt(nonceID)))
         {
            authElement->LinkEndChild(new TiXmlText("true"));
            StringList failedNodes;
            SetupFunctions::startService(service, &failedNodes);
            TiXmlElement *failedHostsElement = new TiXmlElement("failedHosts");
            rootElement->LinkEndChild(failedHostsElement);

            for (StringListIter iter = failedNodes.begin();
               iter != failedNodes.end(); iter++)
            {
               TiXmlElement *nodeElement = new TiXmlElement("node");
               nodeElement->LinkEndChild(new TiXmlText(*iter));
               failedHostsElement->LinkEndChild(nodeElement);
            }
         }
         else
         {
            authElement->LinkEndChild(new TiXmlText("false"));
         }
      }
      else
      if (StringTk::strToBool(stopAll))
      {
         Auth::init();

         if (GUITk::checkReply(secret, Auth::getPassword("admin"),
            StringTk::strToInt(nonceID)))
         {
            authElement->LinkEndChild(new TiXmlText("true"));
            StringList failedNodes;
            SetupFunctions::stopService(service, &failedNodes);
            TiXmlElement *failedHostsElement = new TiXmlElement("failedHosts");
            rootElement->LinkEndChild(failedHostsElement);

            for (StringListIter iter = failedNodes.begin();
               iter != failedNodes.end(); iter++)
            {
               TiXmlElement *nodeElement = new TiXmlElement("node");
               nodeElement->LinkEndChild(new TiXmlText(*iter));
               failedHostsElement->LinkEndChild(nodeElement);
            }
         }
         else
         {
            authElement->LinkEndChild(new TiXmlText("false"));
         }
      }
      else
      if (StringTk::strToBool(start))
      {
         Auth::init();

         if (GUITk::checkReply(secret, Auth::getPassword("admin"),
            StringTk::strToInt(nonceID)))
         {
            authElement->LinkEndChild(new TiXmlText("true"));

            if (SetupFunctions::startService(service, host) != 0)
            {
               TiXmlElement *failedHostsElement =
                  new TiXmlElement("failedHosts");
               rootElement->LinkEndChild(failedHostsElement);
               TiXmlElement *nodeElement = new TiXmlElement("node");
               nodeElement->LinkEndChild(new TiXmlText(host));
               failedHostsElement->LinkEndChild(nodeElement);
            }
         }
         else
         {
            authElement->LinkEndChild(new TiXmlText("false"));
         }
      }
      else
      if (StringTk::strToBool(stop))
      {
         Auth::init();

         if (GUITk::checkReply(secret, Auth::getPassword("admin"),
            StringTk::strToInt(nonceID)))
         {
            authElement->LinkEndChild(new TiXmlText("true"));

            if (SetupFunctions::stopService(service, host) != 0)
            {
               TiXmlElement *failedHostsElement =
                  new TiXmlElement("failedHosts");
               rootElement->LinkEndChild(failedHostsElement);
               TiXmlElement *nodeElement = new TiXmlElement("node");
               nodeElement->LinkEndChild(new TiXmlText(host));
               failedHostsElement->LinkEndChild(nodeElement);
            }
         }
         else
         {
            authElement->LinkEndChild(new TiXmlText("false"));
         }
      }
      else
      {
         authElement->LinkEndChild(new TiXmlText("true"));

         StringList runningHosts;
         StringList stoppedHosts;
         SetupFunctions::checkStatus(service, &runningHosts, &stoppedHosts);
         TiXmlElement *runningElement = new TiXmlElement("runningHosts");
         rootElement->LinkEndChild(runningElement);
         for (StringListIter iter = runningHosts.begin();
            iter != runningHosts.end(); iter++)
         {
            TiXmlElement *nodeElement = new TiXmlElement("node");
            nodeElement->LinkEndChild(new TiXmlText(*iter));
            runningElement->LinkEndChild(nodeElement);
         }

         TiXmlElement *stoppedElement = new TiXmlElement("stoppedHosts");
         rootElement->LinkEndChild(stoppedElement);
         for (StringListIter iter = stoppedHosts.begin();
            iter != stoppedHosts.end(); iter++)
         {
            TiXmlElement *nodeElement = new TiXmlElement("node");
            nodeElement->LinkEndChild(new TiXmlText(*iter));
            stoppedElement->LinkEndChild(nodeElement);
         }
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * gets the details of one storage node
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::storageNode(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   Display display(Program::getApp()->getMetaNodes(),
      Program::getApp()->getStorageNodes(),
      Program::getApp()->getMgmtNodes());
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string node = WebTk::getVar(conn, "node", postBuf, postBufLen);
      uint16_t nodeNumID = StringTk::strToUInt(WebTk::getVar(conn, "nodeNumID", postBuf,
         postBufLen) );
      uint64_t timeSpanPerf = StringTk::strToUInt64(
         WebTk::getVar(conn, "timeSpanPerf", "10", postBuf, postBufLen).c_str());

      long endTimestamp;
      long startTimestamp = calculateStartTime(timeSpanPerf, endTimestamp);

      TiXmlElement *generalElement = new TiXmlElement("general");
      rootElement->LinkEndChild(generalElement);

      TiXmlElement *nodeIDElement = new TiXmlElement("nodeID");
      nodeIDElement->LinkEndChild(new TiXmlText(node));
      generalElement->LinkEndChild(nodeIDElement);
      TiXmlElement *nodeNumIDElement = new TiXmlElement("nodeNumID");
      nodeNumIDElement->LinkEndChild(new TiXmlText(StringTk::uintToStr(nodeNumID)));
      generalElement->LinkEndChild(nodeNumIDElement);

      TiXmlElement *statusElement = new TiXmlElement("status");
      generalElement->LinkEndChild(statusElement);

      std::string outInfo;
      if (display.statusStorage(nodeNumID, &outInfo))
      {
         statusElement->LinkEndChild(new TiXmlText("true"));
      }
      else
      {
         statusElement->LinkEndChild(new TiXmlText("false"));
      }

      GeneralNodeInfo info;

      if (display.getGeneralStorageNodeInfo(nodeNumID, &info))
      {
         TiXmlElement *cpuModelElement = new TiXmlElement("cpuModel");
         cpuModelElement->LinkEndChild(new TiXmlText(info.cpuName));
         generalElement->LinkEndChild(cpuModelElement);

         TiXmlElement *cpuCountElement = new TiXmlElement("cpuCount");
         cpuCountElement->LinkEndChild(new TiXmlText(
            StringTk::intToStr(info.cpuCount)));
         generalElement->LinkEndChild(cpuCountElement);

         char cpuSpeedStr[32];
         sprintf(cpuSpeedStr, "%0.1lf", (double) (info.cpuSpeed) / 1000);

         TiXmlElement *cpuSpeedElement = new TiXmlElement("cpuSpeed");
         cpuSpeedElement->LinkEndChild(new TiXmlText(
            std::string(cpuSpeedStr) + "GHz"));
         generalElement->LinkEndChild(cpuSpeedElement);

         char ramStr[32];
         sprintf(ramStr, "%0.1lf", (double) (info.memTotal) / 1000);
         TiXmlElement *ramSizeElement = new TiXmlElement("ramSize");
         ramSizeElement->LinkEndChild(
            new TiXmlText(std::string(ramStr) + "GB"));
         generalElement->LinkEndChild(ramSizeElement);
      }
      else
      {
         TiXmlElement *cpuModelElement = new TiXmlElement("cpuModel");
         cpuModelElement->LinkEndChild(new TiXmlText(""));
         generalElement->LinkEndChild(cpuModelElement);

         TiXmlElement *cpuCountElement = new TiXmlElement("cpuCount");
         cpuCountElement->LinkEndChild(new TiXmlText(""));
         generalElement->LinkEndChild(cpuCountElement);

         TiXmlElement *cpuSpeedElement = new TiXmlElement("cpuSpeed");
         cpuSpeedElement->LinkEndChild(new TiXmlText(""));
         generalElement->LinkEndChild(cpuSpeedElement);

         TiXmlElement *ramSizeElement = new TiXmlElement("ramSize");
         ramSizeElement->LinkEndChild(new TiXmlText(""));
         generalElement->LinkEndChild(ramSizeElement);
      }

      TiXmlElement *lastMessageElement = new TiXmlElement("lastMessage");
      lastMessageElement->LinkEndChild(new TiXmlText(
         display.timeSinceLastMessageStorage(nodeNumID)));
      generalElement->LinkEndChild(lastMessageElement);

      TiXmlElement *openSessionsElement = new TiXmlElement("openSessions");
      openSessionsElement->LinkEndChild(new TiXmlText(StringTk::uintToStr(
         display.sessionCountStorage(nodeNumID))));
      generalElement->LinkEndChild(openSessionsElement);

      TiXmlElement *storageTargetsElement = new TiXmlElement("storageTargets");
      rootElement->LinkEndChild(storageTargetsElement);
      StorageTargetInfoList storageTargets;
      display.storageTargetsInfo(nodeNumID, &storageTargets);

      for (StorageTargetInfoListIter iter = storageTargets.begin();
         iter != storageTargets.end(); iter++)
      {
         StorageTargetInfo targetInfo = *iter;
         TiXmlElement *targetElement = new TiXmlElement("target");
         storageTargetsElement->LinkEndChild(targetElement);
         targetElement->LinkEndChild(new TiXmlText(StringTk::uintToStr(targetInfo.getTargetID())));
         targetElement->SetAttribute("diskSpaceTotal", StringTk::int64ToStr(
            targetInfo.getDiskSpaceTotal()));
         targetElement->SetAttribute("diskSpaceFree",
            StringTk::int64ToStr(targetInfo.getDiskSpaceFree()));
         targetElement->SetAttribute("pathStr", targetInfo.getPathStr());
      }

      std::string unit;

      TiXmlElement *diskPerfSummaryElement =
         new TiXmlElement("diskPerfSummary");
      rootElement->LinkEndChild(diskPerfSummaryElement);

      TiXmlElement *diskReadSumElement = new TiXmlElement("diskReadSum");
      diskReadSumElement->LinkEndChild(new TiXmlText(display.diskRead(nodeNumID,
         timeSpanPerf, &unit) + " " + unit));
      diskPerfSummaryElement->LinkEndChild(diskReadSumElement);

      TiXmlElement *diskWriteSumElement = new TiXmlElement("diskWriteSum");
      diskWriteSumElement->LinkEndChild(new TiXmlText(display.diskWrite(nodeNumID,
         timeSpanPerf, &unit) + " " + unit));
      diskPerfSummaryElement->LinkEndChild(diskWriteSumElement);

      UInt64List timesRead;
      UInt64List read;
      UInt64List timesAverageRead;
      UInt64List averageRead;
      display.diskPerfRead(nodeNumID, timeSpanPerf, &timesRead, &read,
         &timesAverageRead, &averageRead, startTimestamp, endTimestamp);

      UInt64List timesWrite;
      UInt64List write;
      UInt64List timesAverageWrite;
      UInt64List averageWrite;
      display.diskPerfWrite(nodeNumID, timeSpanPerf, &timesWrite, &write,
         &timesAverageWrite, &averageWrite, startTimestamp, endTimestamp);

      TiXmlElement *diskPerfReadElement = new TiXmlElement("diskPerfRead");
      rootElement->LinkEndChild(diskPerfReadElement);

      UInt64ListIter timesIter = timesRead.begin();
      UInt64ListIter valuesIter = read.begin();
      while ((timesIter != timesRead.end()) && (valuesIter != read.end()))
      {
         TiXmlElement *valueElement = new TiXmlElement("value");
         valueElement->SetAttribute("time", StringTk::uint64ToStr(*timesIter));
         valueElement->LinkEndChild(new TiXmlText(StringTk::uint64ToStr(*valuesIter)));
         diskPerfReadElement->LinkEndChild(valueElement);
         timesIter++;
         valuesIter++;
      }

      TiXmlElement *diskPerfAverageReadElement =
         new TiXmlElement("diskPerfAverageRead");
      rootElement->LinkEndChild(diskPerfAverageReadElement);

      timesIter = timesAverageRead.begin();
      valuesIter = averageRead.begin();
      while ((timesIter != timesAverageRead.end()) &&
         (valuesIter != averageRead.end()))
      {
         TiXmlElement *valueElement = new TiXmlElement("value");
         valueElement->SetAttribute("time", StringTk::uint64ToStr(*timesIter));
         valueElement->LinkEndChild(new TiXmlText(StringTk::uint64ToStr(*valuesIter)));
         diskPerfAverageReadElement->LinkEndChild(valueElement);
         timesIter++;
         valuesIter++;
      }

      TiXmlElement *diskPerfWriteElement = new TiXmlElement("diskPerfWrite");
      rootElement->LinkEndChild(diskPerfWriteElement);

      timesIter = timesWrite.begin();
      valuesIter = write.begin();
      while ((timesIter != timesWrite.end()) && (valuesIter != write.end()))
      {
         TiXmlElement *valueElement = new TiXmlElement("value");
         valueElement->SetAttribute("time", StringTk::uint64ToStr(*timesIter));
         valueElement->LinkEndChild(new TiXmlText(StringTk::uint64ToStr(*valuesIter)));
         diskPerfWriteElement->LinkEndChild(valueElement);
         timesIter++;
         valuesIter++;
      }

      TiXmlElement *diskPerfAverageWriteElement =
         new TiXmlElement("diskPerfAverageWrite");
      rootElement->LinkEndChild(diskPerfAverageWriteElement);

      timesIter = timesAverageWrite.begin();
      valuesIter = averageWrite.begin();
      while ((timesIter != timesAverageWrite.end()) &&
         (valuesIter != averageWrite.end()))
      {
         TiXmlElement *valueElement = new TiXmlElement("value");
         valueElement->SetAttribute("time", StringTk::uint64ToStr(*timesIter));
         valueElement->LinkEndChild(new TiXmlText(StringTk::uint64ToStr(*valuesIter)));
         diskPerfAverageWriteElement->LinkEndChild(valueElement);
         timesIter++;
         valuesIter++;
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/*
 * gets the overview about all metadata nodes
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::storageNodesOverview(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   Display display(Program::getApp()->getMetaNodes(), Program::getApp()->getStorageNodes(),
      Program::getApp()->getMgmtNodes());
   Database *db = Program::getApp()->getDB();
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   try
   {
      std::string group = WebTk::getVar(conn, "group", "", postBuf, postBufLen);
      uint timeSpanPerf = StringTk::strToUInt(
         WebTk::getVar(conn, "timeSpanPerf", "10", postBuf, postBufLen) );

      long endTimestamp;
      long startTimestamp = calculateStartTime(timeSpanPerf, endTimestamp);

      StringList nodes;
      UInt16List nodeNumIDs;
      if (group.compare("") == 0)
      {
         Program::getApp()->getStorageNodesAsStringList(&nodes);
         Program::getApp()->getStorageNodeNumIDs(&nodeNumIDs);
      }
      else
      {
         db->getStorageNodesInGroup(group, &nodes);
         db->getStorageNodesInGroup(group, &nodeNumIDs);
      }

      TiXmlElement *statusElement = new TiXmlElement("status");
      rootElement->LinkEndChild(statusElement);

      UInt16ListIter nodesNumIDIter = nodeNumIDs.begin();
      for (StringListIter nodesIter = nodes.begin(); nodesIter != nodes.end(); nodesIter++,
         nodesNumIDIter++)
      {
         std::string outInfo;
         std::string nodeID = *nodesIter;
         uint16_t nodeNumID = *nodesNumIDIter;

         TiXmlElement *valueElement = new TiXmlElement("value");
         statusElement->LinkEndChild(valueElement);
         valueElement->SetAttribute("node", nodeID);
         valueElement->SetAttribute("nodeNumID", nodeNumID);

         if (display.statusStorage(nodeNumID, &outInfo))
            valueElement->LinkEndChild(new TiXmlText("true"));
         else
            valueElement->LinkEndChild(new TiXmlText("false"));
      }

      TiXmlElement *diskSpaceElement = new TiXmlElement("diskSpace");
      rootElement->LinkEndChild(diskSpaceElement);
      std::string unit;

      TiXmlElement *diskSpaceTotalElement =
         new TiXmlElement("diskSpaceTotal");
      diskSpaceTotalElement->LinkEndChild(new TiXmlText(
         display.diskSpaceTotalSum(&unit) + " " + unit));
      diskSpaceElement->LinkEndChild(diskSpaceTotalElement);

      TiXmlElement *diskSpaceFreeElement = new TiXmlElement("diskSpaceFree");
      diskSpaceFreeElement->LinkEndChild(
         new TiXmlText(display.diskSpaceFreeSum(&unit) + " " + unit));
      diskSpaceElement->LinkEndChild(diskSpaceFreeElement);

      TiXmlElement *diskSpaceUsedElement = new TiXmlElement("diskSpaceUsed");
      diskSpaceUsedElement->LinkEndChild(
         new TiXmlText(display.diskSpaceUsedSum(&unit) + " " + unit));
      diskSpaceElement->LinkEndChild(diskSpaceUsedElement);

      TiXmlElement *diskPerfSummaryElement =
         new TiXmlElement("diskPerfSummary");
      rootElement->LinkEndChild(diskPerfSummaryElement);

      TiXmlElement *diskReadSumElement = new TiXmlElement("diskReadSum");
      diskReadSumElement->LinkEndChild(new TiXmlText(display.diskReadSum(
         timeSpanPerf, &unit) + " " + unit));
      diskPerfSummaryElement->LinkEndChild(diskReadSumElement);

      TiXmlElement *diskWriteSumElement = new TiXmlElement("diskWriteSum");
      diskWriteSumElement->LinkEndChild(new TiXmlText(display.diskWriteSum(
         timeSpanPerf, &unit) + " " + unit));

      diskPerfSummaryElement->LinkEndChild(diskWriteSumElement);

      UInt64List timesRead;
      UInt64List read;
      UInt64List timesAverageRead;
      UInt64List averageRead;
      display.diskPerfReadSum(timeSpanPerf, &timesRead, &read,
         &timesAverageRead, &averageRead, startTimestamp, endTimestamp);

      UInt64List timesWrite;
      UInt64List write;
      UInt64List timesAverageWrite;
      UInt64List averageWrite;
      display.diskPerfWriteSum(timeSpanPerf, &timesWrite, &write,
         &timesAverageWrite, &averageWrite, startTimestamp, endTimestamp);

      TiXmlElement *diskPerfReadElement = new TiXmlElement("diskPerfRead");
      rootElement->LinkEndChild(diskPerfReadElement);

      UInt64ListIter timesIter = timesRead.begin();
      UInt64ListIter valuesIter = read.begin();
      while ((timesIter != timesRead.end()) && (valuesIter != read.end()))
      {
         TiXmlElement *valueElement = new TiXmlElement("value");
         valueElement->SetAttribute("time", StringTk::uint64ToStr(*timesIter));
         valueElement->LinkEndChild(new TiXmlText(StringTk::uint64ToStr(*valuesIter)));
         diskPerfReadElement->LinkEndChild(valueElement);
         timesIter++;
         valuesIter++;
      }

      TiXmlElement *diskPerfAverageReadElement =
         new TiXmlElement("diskPerfAverageRead");
      rootElement->LinkEndChild(diskPerfAverageReadElement);
      timesIter = timesAverageRead.begin();
      valuesIter = averageRead.begin();
      while ((timesIter != timesAverageRead.end()) &&
         (valuesIter != averageRead.end()))
      {
         TiXmlElement *valueElement = new TiXmlElement("value");
         valueElement->SetAttribute("time", StringTk::uint64ToStr(*timesIter));
         valueElement->LinkEndChild(new TiXmlText(StringTk::uint64ToStr(*valuesIter)));
         diskPerfAverageReadElement->LinkEndChild(valueElement);
         timesIter++;
         valuesIter++;
      }

      TiXmlElement *diskPerfWriteElement = new TiXmlElement("diskPerfWrite");
      rootElement->LinkEndChild(diskPerfWriteElement);
      timesIter = timesWrite.begin();
      valuesIter = write.begin();
      while ((timesIter != timesWrite.end()) && (valuesIter != write.end()))
      {
         TiXmlElement *valueElement = new TiXmlElement("value");
         valueElement->SetAttribute("time", StringTk::uint64ToStr(*timesIter));
         valueElement->LinkEndChild(new TiXmlText(StringTk::uint64ToStr(*valuesIter)));
         diskPerfWriteElement->LinkEndChild(valueElement);
         timesIter++;
         valuesIter++;
      }

      TiXmlElement *diskPerfAverageWriteElement =
         new TiXmlElement("diskPerfAverageWrite");
      rootElement->LinkEndChild(diskPerfAverageWriteElement);

      timesIter = timesAverageWrite.begin();
      valuesIter = averageWrite.begin();
      while ((timesIter != timesAverageWrite.end()) &&
         (valuesIter != averageWrite.end()))
      {
         TiXmlElement *valueElement = new TiXmlElement("value");
         valueElement->SetAttribute("time", StringTk::uint64ToStr(*timesIter));
         valueElement->LinkEndChild(new TiXmlText(StringTk::uint64ToStr(*valuesIter)));
         diskPerfAverageWriteElement->LinkEndChild(valueElement);
         timesIter++;
         valuesIter++;
      }

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/**
 * Gets and sets a stripe pattern for a path (depending on whether
 * "change" is set).
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::striping(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   App* app = Program::getApp();
   Logger* log = app->getLogger();

   NodeStoreMgmtEx* mgmtNodes = app->getMgmtNodes();
   NodeStoreMetaEx* metaNodes = app->getMetaNodes();
   NodeStoreStorageEx* storageNodes = app->getStorageNodes();
   TargetMapper* targetMapper = app->getTargetMapper();

   Display display(metaNodes, storageNodes, Program::getApp()->getMgmtNodes() );
      
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   bool responseErr = false; // will be contained in the error element of the response

   try
   {
      std::string pathStr = WebTk::getVar(conn, "pathStr", "/", postBuf, postBufLen);
      std::string change = WebTk::getVar(conn, "change", "false", postBuf, postBufLen);
      std::string dnn = WebTk::getVar(conn, "dnn", "0", postBuf, postBufLen);
      std::string cs = WebTk::getVar(conn, "cs", "0", postBuf, postBufLen);
      std::string metaMirroring = WebTk::getVar(conn, "metaMirroring", "false", postBuf,
         postBufLen);
      std::string patternID = WebTk::getVar(conn, "pattern", "0", postBuf, postBufLen);
      std::string nonceID = WebTk::getVar(conn, "nonceID", "-1", postBuf, postBufLen);
      std::string secret = WebTk::getVar(conn, "secret", "", postBuf, postBufLen);

      // note: the change element defines whether we only get or set+get a stripe pattern with this

      if ((StringTk::strToBool(change)) && (pathStr.compare("") != 0))
      { // set new stripe pattern
         TiXmlElement *authElement = new TiXmlElement("authenticated");
         rootElement->LinkEndChild(authElement);
         Auth::init();
         if (GUITk::checkReply(secret, Auth::getPassword("admin"),
            StringTk::strToInt(nonceID)))
         {
            authElement->LinkEndChild(new TiXmlText("true"));
            bool setPatternRes = display.setPattern(pathStr,
               StringTk::strToUInt(cs), StringTk::strToUInt(dnn),
               StringTk::strToUInt(patternID), StringTk::strToBool(metaMirroring));

            if(!setPatternRes)
               responseErr = true; // setting of pattern failed
         }
         else
            authElement->LinkEndChild(new TiXmlText("false") );
      }
      else
      { // retrieve current stripe pattern
         TiXmlElement *settingsElement = new TiXmlElement("settings");
         rootElement->LinkEndChild(settingsElement);

         if(!pathStr.empty() )
         {
            std::string chunkSize;
            std::string defaultNumNodes;
            UInt16Vector currentTargetNumIDs;
            UInt16Vector currentMirrorTargetNumIDs;
            UInt16Vector currentBMGIDs;
            uint16_t currentMetaNodeNumID;
            uint16_t currentMirrorMetaNodeNumID;
            unsigned pattern;

            bool getInfoRes = display.getEntryInfo(pathStr, &chunkSize,
               &defaultNumNodes, &currentTargetNumIDs, &pattern, &currentMirrorTargetNumIDs,
               &currentBMGIDs, &currentMetaNodeNumID, &currentMirrorMetaNodeNumID);

            if(!getInfoRes)
               responseErr = true; // error on entry info retrieval

            // if path has no storage nodes assigned, assume it's a directory
            bool isDir = currentTargetNumIDs.empty();

            TiXmlElement *pathElement = new TiXmlElement("path");
            settingsElement->LinkEndChild(pathElement);
            pathElement->LinkEndChild(new TiXmlText(pathStr) );

            TiXmlElement *dirElement = new TiXmlElement("isDir");
            settingsElement->LinkEndChild(dirElement);
            dirElement->LinkEndChild(new TiXmlText(isDir ? "true" : "false") );

            TiXmlElement *chunksizeElement = new TiXmlElement("chunksize");
            settingsElement->LinkEndChild(chunksizeElement);
            chunksizeElement->LinkEndChild(new TiXmlText(chunkSize) );

            TiXmlElement *patternElement = new TiXmlElement("pattern");
            settingsElement->LinkEndChild(patternElement);
            patternElement->LinkEndChild(new TiXmlText(StringTk::uintToStr(pattern)));

            TiXmlElement *defaultNumNodesElement = new TiXmlElement("defaultNumNodes");
            settingsElement->LinkEndChild(defaultNumNodesElement);
            defaultNumNodesElement->LinkEndChild(
               new TiXmlText(defaultNumNodes) );

            TiXmlElement *metaElement = new TiXmlElement("meta");
            rootElement->LinkEndChild(metaElement);
            metaElement->LinkEndChild(new TiXmlText(
               metaNodes->getNodeIDWithTypeStr(currentMetaNodeNumID) ) );

            TiXmlElement *metaMirroringElement = new TiXmlElement("metaMirroring");
            settingsElement->LinkEndChild(metaMirroringElement);

            TiXmlElement *metaMirrorElement = new TiXmlElement("metaMirror");
            rootElement->LinkEndChild(metaMirrorElement);

            if(currentMirrorMetaNodeNumID)
            {
               metaMirrorElement->LinkEndChild(new TiXmlText(
                  metaNodes->getNodeIDWithTypeStr(currentMirrorMetaNodeNumID) ) );
               metaMirroringElement->LinkEndChild(new TiXmlText("true"));
            }
            else
            {
               metaMirrorElement->LinkEndChild(new TiXmlText("") );
               metaMirroringElement->LinkEndChild(new TiXmlText("false"));
            }

            if(!isDir)
            { // file => add stripe nodes
               UInt16List mappedTargetIDs;
               UInt16List mappedNodeIDs;
               bool targetMappingSuccess = false;

               Node* mgmtNode = mgmtNodes->referenceFirstNode();

               if( (mgmtNode != NULL) &&
                  NodesTk::downloadTargetMappings(mgmtNode, &mappedTargetIDs, &mappedNodeIDs,
                     false) )
               {
                  targetMapper->syncTargetsFromLists(mappedTargetIDs, mappedNodeIDs);
                  targetMappingSuccess = true;
               }
               else
               {
                  log->log(Log_ERR, __func__, "Failed to download target mappings from mgmtd.");
               }

               TiXmlElement *targetElement = new TiXmlElement("storageTargets");
               rootElement->LinkEndChild(targetElement);
               for(UInt16VectorIter iter = currentTargetNumIDs.begin();
                  iter != currentTargetNumIDs.end(); iter++)
               {
                  std::string node = "<unknown>";
                  if(targetMappingSuccess)
                  {
                     uint16_t nodeID = targetMapper->getNodeID(*iter);
                     node = storageNodes->getNodeIDWithTypeStr(nodeID);
                  }

                  TiXmlElement *nodeElement = new TiXmlElement("target");
                  nodeElement->LinkEndChild(new TiXmlText(
                     StringTk::uintToStr(*iter) + " @ " + node) );
                  targetElement->LinkEndChild(nodeElement);
               }

               TiXmlElement *mirrorTargetElement = new TiXmlElement("mirrorStorageTargets");
               rootElement->LinkEndChild(mirrorTargetElement);
               for(UInt16VectorIter iter = currentMirrorTargetNumIDs.begin();
                  iter != currentMirrorTargetNumIDs.end(); iter++)
               {
                  std::string node = "<unknown>";
                  if(targetMappingSuccess)
                  {
                     uint16_t nodeID = targetMapper->getNodeID(*iter);
                     node = storageNodes->getNodeIDWithTypeStr(nodeID);
                  }

                  TiXmlElement *nodeElement = new TiXmlElement("target");
                  nodeElement->LinkEndChild(new TiXmlText(
                     StringTk::uintToStr(*iter) + " @ " + node) );
                  mirrorTargetElement->LinkEndChild(nodeElement);
               }

               TiXmlElement *mirrorBuddyGroupsElement =
                  new TiXmlElement("storageMirrorBuddyGroups");
               rootElement->LinkEndChild(mirrorBuddyGroupsElement);
               for(UInt16VectorIter iter = currentBMGIDs.begin(); iter != currentBMGIDs.end();
                  iter++)
               {
                  TiXmlElement *nodeElement = new TiXmlElement("id");
                  nodeElement->LinkEndChild(new TiXmlText(StringTk::uintToStr(*iter) ) );
                  mirrorBuddyGroupsElement->LinkEndChild(nodeElement);
               }
            }
         }
      }

      // add the general error element
      TiXmlElement* errorElement = new TiXmlElement("error");
      rootElement->LinkEndChild(errorElement);
      errorElement->LinkEndChild(
         new TiXmlText(responseErr ? "true" : "false") );

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}

/**
 * gets the beegfs version
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::getVersionString(struct mg_connection *conn,
   const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif
   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   TiXmlElement *element = new TiXmlElement("version");
   TiXmlText *elementText = new TiXmlText(BEEGFS_VERSION);
   element->LinkEndChild(elementText);
   rootElement->LinkEndChild(element);

   XmlCreator::writeDoc(conn, request_info, doc);
}

/**
 * send a test eMail to test the eMail notification settings
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::testEMail(struct mg_connection *conn,
   const struct mg_request_info * request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   App* app  = Program::getApp();
   Mailer* mailer = app->getMailer();

   std::string msg = "Test email from beegfs-admon.";
   std::string subject = "BeeGFS : test email";

   int sendToSmtpServerSuccsessfull = mailer->sendMail(subject, msg);

   TiXmlDocument doc;
   TiXmlElement *rootElement = new TiXmlElement("data");
   doc.LinkEndChild(rootElement);

   TiXmlElement *successElement = new TiXmlElement("success");
   rootElement->LinkEndChild(successElement);

   if (sendToSmtpServerSuccsessfull == 0)
   {
      TiXmlText *successElementText = new TiXmlText("true");
      successElement->LinkEndChild(successElementText);
   }
   else
   {
      TiXmlText *successElementText = new TiXmlText("false");
      successElement->LinkEndChild(successElementText);
   }

   XmlCreator::writeDoc(conn, request_info, doc);
}

/**
 * Collects the client stats.
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::clientStats(struct mg_connection *conn,
   const struct mg_request_info * request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn, request_info, postBuf, postBufLen);
#endif

   XmlCreator::stats(false, conn, request_info, postBuf, postBufLen);
}

/**
 * Collects the user stats.
 *
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::userStats(struct mg_connection *conn,
   const struct mg_request_info * request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn, request_info, postBuf, postBufLen);
#endif

   XmlCreator::stats(true, conn, request_info, postBuf, postBufLen);
}

/**
 * Collects the client stats.
 *
 * @param doUserStats if true user stats are requested, if false client stats are requested
 * @param conn The mg_connection from mongoose
 * @param request_info The mg_request_info from mongoose
 * @param postBuf A buffer which contains the POST content
 * @param postBufLen The length of the POST buffer
 */
void XmlCreator::stats(bool doUserStats, struct mg_connection *conn,
   const struct mg_request_info * request_info, const char* postBuf, size_t postBufLen)
{
#ifdef MONGOOSE_DEBUG
   debugMongoose(conn,request_info, postBuf, postBufLen);
#endif

   StatsOperator* cStatsOperator = Program::getApp()->getClientStatsOperator();

   try
   {
      uint64_t currentDataSequenceID;

      int type = StringTk::strToInt(WebTk::getVar(conn, "nodetype", "1", postBuf,
         postBufLen) );

      NodeType nodeType;
      if(type == NODETYPE_Meta)
         nodeType = NODETYPE_Meta;
      else if(type == NODETYPE_Storage)
         nodeType = NODETYPE_Storage;
      else
         nodeType = NODETYPE_Invalid;

      unsigned intervalSecs = StringTk::strToUInt(WebTk::getVar(conn, "interval", "10", postBuf,
         postBufLen) );
      unsigned numClients = StringTk::strToUInt(WebTk::getVar(conn, "numLines", "10", postBuf,
         postBufLen) );
      unsigned requestorID = StringTk::strToUInt(WebTk::getVar(conn, "requestorID", "0", postBuf,
         postBufLen) );
      uint64_t nextDataSequenceID = StringTk::strToUInt64(WebTk::getVar(conn, "nextDataSequenceID",
         postBuf, postBufLen).c_str() );

      if (cStatsOperator->needUpdate(requestorID, intervalSecs, numClients, nodeType, doUserStats) )
      {
         requestorID = cStatsOperator->addOrUpdate(requestorID, intervalSecs, numClients, nodeType,
            doUserStats);
      }

      StatsAdmon* stats = cStatsOperator->getStats(requestorID, currentDataSequenceID);

      TiXmlDocument doc;

      TiXmlElement *rootElement = new TiXmlElement("data");
      doc.LinkEndChild(rootElement);

      TiXmlElement *requestorIDElement = new TiXmlElement("requestorID");
      rootElement->LinkEndChild(requestorIDElement);
      requestorIDElement->LinkEndChild(new TiXmlText(StringTk::uintToStr(requestorID) ) );

      TiXmlElement *dataSequenceIDElement = new TiXmlElement("dataSequenceID");
      rootElement->LinkEndChild(dataSequenceIDElement);
      dataSequenceIDElement->LinkEndChild(new TiXmlText(StringTk::uint64ToStr(
         currentDataSequenceID)));

      if (nextDataSequenceID <= currentDataSequenceID)
         stats->addStatsToXml(rootElement, doUserStats);

      SAFE_DELETE(stats);

      XmlCreator::writeDoc(conn, request_info, doc);
   }
   catch (HttpVarException &e)
   {
      XmlCreator::sendError(conn, request_info, e.what());
   }
}
