#ifndef XMLCREATOR_H_
#define XMLCREATOR_H_

/*
 * XmlCreator is a collection of a lot static functions used as callback functions by WebServer.
 * The class makes use of "Tiny XML C++" (http://code.google.com/p/ticpp/), which is directly
 * included in thirdparty/ticpp, to create XML pages, that are delivered to the GUI.
 *
 * Example usage of ticcp in XMLCreator functions:
 *
 * // create XML document
 * TiXmlDocument doc;
 *
 * //create a root element with the name data
 * TiXmlElement *rootElement = new TiXmlElement("data");
 *
 * // link the rootElement to the doc (each node needs to be linked to a parent)
 * doc.LinkEndChild(rootElement);
 *
 * // create another node, named foo and link it to the rootElement
 * TiXmlElement *fooElement = new TiXmlElement("foo");
 * rootElement->LinkEndChild(fooElement);
 *
 * // create a TextElement and directly link it to the rootElement
 * fooElement->LinkEndChild(new TiXmlText("Text for foo"));
 *
 *
 *
 * This function creates the following XML document :
 * <data>
 *   <foo>Text for foo</foo>
 * </data>
 *
 * The document can be printed to the mongoose output stream using
 * XmlCreator::writeDoc() afterwards
 */

#include <common/toolkit/StringTk.h>
#include <toolkit/GUITk.h>
#include <toolkit/WebTk.h>
#include <setup/SetupFunctions.h>
#include <web/auth/Auth.h>
#include <web/displayFunctions/Display.h>

#include <fstream>
#include <grp.h>
#include <mongoose.h>
#include <pwd.h>
#include <sstream>
#include <ticpp.h>


#define MONGOOSE_DEBUG

class XmlCreator
{
   public:
      XmlCreator() {}

      static void debugMongoose(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);

      static void writeDoc(struct mg_connection *conn, const struct mg_request_info *request_info,
         TiXmlDocument doc);

      static void serveFile(struct mg_connection *conn, const struct mg_request_info *request_info,
         std::string filename);

      static void admonGui(struct mg_connection *conn,
         const struct mg_request_info *request_info);

      static void sendError(struct mg_connection *conn, const struct mg_request_info *request_info,
         std::string errorMessage);
      static void sendHttpError(struct mg_connection *conn,
         const struct mg_request_info *request_info, const int errorCode, std::string errorMessage);

      static void testXML(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void getNonce(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void authInfo(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void changePassword(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void checkDistriArch(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void checkSSH(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void configBasicConfig(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void configIBConfig(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void configRoles(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void downloadRelease(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void fileBrowser(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void getCheckClientDependenciesLog(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void getGroups(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void getRemoteLogFile(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void install(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void installInfo(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void uninstall(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void knownProblems(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void getLocalLogFile(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void mailNotification(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void metaNode(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void metaNodesOverview(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void nodeList(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void preAuthInfo(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void scriptSettings(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void createNewSetupLogfile(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void startStop(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void storageNode(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void storageNodesOverview(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void striping(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void clientStats(struct mg_connection *conn,
         const struct mg_request_info * request_info, const char* postBuf, size_t postBufLen);
      static void userStats(struct mg_connection *conn,
         const struct mg_request_info * request_info, const char* postBuf, size_t postBufLen);
      static void stats(bool doUserStats, struct mg_connection *conn,
         const struct mg_request_info * request_info, const char* postBuf, size_t postBufLen);
      static void getVersionString(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);
      static void testEMail(struct mg_connection *conn,
         const struct mg_request_info *request_info, const char* postBuf, size_t postBufLen);

   private:

      static inline long calculateStartTime(uint64_t timespanMinutes, long &timestampEndMS)
      {
         TimeAbs t;
         t.setToNow();
         timestampEndMS = t.getTimeMS();
         return (t.getTimeMS() - (timespanMinutes * 60 * 1000));
      }
};

#endif /* XMLCREATOR_H_ */
