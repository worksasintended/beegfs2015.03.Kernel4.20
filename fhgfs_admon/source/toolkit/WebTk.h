/*
 * static helper functions for dealing with http variables (parsing the HTTP variables that were
 * passed to mongoose) in several ways
 */

#include <common/toolkit/StringTk.h>
#include <toolkit/HttpVarException.h>

#include <mongoose.h>
#include <string>


#ifndef WEBTK_H_
#define WEBTK_H_

class WebTk
{
   public:
      WebTk();
      virtual ~WebTk();
      static std::string getVar(struct mg_connection *conn, std::string name, const char* postBuf,
         size_t postBufLen) throw (HttpVarException);
      static std::string getVar(struct mg_connection *conn, std::string name,std::string defaultVal,
         const char* postBuf, size_t postBufLen);
      static void paramsToMap(struct mg_connection *conn, StringMap *outParamsMap,
         const char* postBuf, size_t postBufLen);
      static void paramsToMap(struct mg_connection *conn, StringMap *outParamsMap,
         std::string ignoreParam, const char* postBuf, size_t postBufLen);
      static void paramsToMap(std::string paramsStr, StringMap *outParamsMap);
      static void getVarList(struct mg_connection *conn, std::string name,
         StringList *outList, const char* postBuf, size_t postBufLen);
      static void getVarList(struct mg_connection *conn, std::string name,
         UInt16List *outList, const char* postBuf, size_t postBufLen);
      static std::string getIP(const struct mg_request_info *request_info);

   private:
      static std::string getVarFromBuf(const char *buf, size_t bufLen, std::string name);
      static std::string getVarFromGet(struct mg_connection *conn, std::string name);
      static std::string getVarFromPost(struct mg_connection *conn, std::string name);
      static std::string getVarStringBuffer(struct mg_connection *conn, const char* postBuf,
         size_t postBufLen);
};

#endif /* WEBTK_H_ */
