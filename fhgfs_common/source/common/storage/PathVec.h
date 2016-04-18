#ifndef PATHVEC_H_
#define PATHVEC_H_

#include <common/app/log/LogContext.h>
#include <common/toolkit/StringTk.h>
#include <common/Common.h>


class PathVec
{
   public:
      PathVec()
      {
         this->isPathAbsolute = false;
         this->isPathStrValid = false;
      }
      
      PathVec(const std::string pathStr)
      {
         this->isPathAbsolute = isPathStrAbsolute(pathStr);
         parseStr(pathStr);
         
         this->isPathStrValid = false;
      }
      
      PathVec(PathVec& parentPath, PathVec& childPath)
      {
         this->isPathAbsolute = parentPath.isPathAbsolute;
         this->isPathStrValid = false;
         
         pathElems.insert(pathElems.end(),
            parentPath.pathElems.begin(), parentPath.pathElems.end() );
         pathElems.insert(pathElems.end(),
            childPath.pathElems.begin(), childPath.pathElems.end() );
      }

      PathVec(PathVec& parentPath, const std::string childPathStr)
      {
         PathVec childPath(childPathStr);
         
         this->isPathAbsolute = parentPath.isPathAbsolute;
         this->isPathStrValid = false;
         
         pathElems.insert(pathElems.end(),
            parentPath.pathElems.begin(), parentPath.pathElems.end() );
         pathElems.insert(pathElems.end(),
            childPath.pathElems.begin(), childPath.pathElems.end() );
      }

      
      void parseStr(const std::string pathStr)
      {
         StringTk::explode(pathStr, '/', &pathElems);
      }
      
      /**
       * Call this to allow usage of getPathVecAsStrConst() later (for thread-safety)
       */
      void initPathStr()
      {
         if(!isPathStrValid)
            updatePathStr();
      }


      
   private:
      StringVector pathElems;
      bool isPathAbsolute;
      
      std::string pathStr;
      bool isPathStrValid;

      // inliners
      static bool isPathStrAbsolute(std::string pathStr)
      {
         return (pathStr.length() && (pathStr[0] == '/') );
      }
      
      void updatePathStr()
      {
         StringVectorIter iter = pathElems.begin();
         size_t pathSize = pathElems.size();
         
         pathStr.clear();
         
         if(isPathAbsolute)
            pathStr += "/";
         
         for(size_t i=0; i < pathSize; i++, iter++)
         {
            if(i)
               pathStr += "/";

            pathStr += *iter;  
         }

         
         isPathStrValid = true;
      }
      
      
   public:
      // inliners
      
      bool operator == (const PathVec& other) const
      {
         if(pathElems.size() != other.pathElems.size() )
            return false;
            
         if(isPathAbsolute != other.isPathAbsolute)
            return false;
            
         StringVectorConstIter iterThis = pathElems.begin();
         StringVectorConstIter iterOther = other.pathElems.begin();
         for( ; iterThis != pathElems.end(); iterThis++, iterOther++)
         {
            if(iterThis->compare(*iterOther) )
               return false;
         }
         
         return true;
      }

      bool operator != (const PathVec& other) const
      {
         return !(*this == other);
      }
      
      void appendElem(std::string newElem)
      {
         pathElems.push_back(newElem);
         isPathStrValid = false;
      }
      
      
      void popLastElem()
      {
         pathElems.pop_back();
         isPathStrValid = false;
      }

      // getters & setters
      
      size_t getNumPathElems()
      {
         return pathElems.size();
      }
      
      bool getIsEmpty()
      {
         return pathElems.empty();
      }


      const StringVector* getPathElems() const
      {
         return &pathElems;
      }

      /**
       * Note: Only use this if you are planning to update the path elements.
       */
      StringVector* getPathElemsNonConst()
      {
         isPathStrValid = false;
         return &pathElems;
      }
      
      std::string getLastElem() const
      {
         StringVectorConstIter iter = --(pathElems.end() );
         return *iter;
      }
      
      /**
       * @return string does not end with a slash
       */
      std::string getPathAsStr()
      {
         if(!isPathStrValid)
            updatePathStr();
         
         return pathStr;
      }

      /**
       * Note: Make sure that the internal string was initialized when you use this (typically by
       * calling initPathVecStr() )
       *
       * @return string does not end with a slash
       */
      std::string getPathAsStrConst() const
      {
         if(unlikely(!isPathStrValid) )
         {
            LogContext logContext(__func__);
            logContext.log(Log_CRITICAL, "BUG(?): called with uninitialized path string");

            LOG_DEBUG_BACKTRACE();

            return "<undef>";
         }

         return pathStr;
      }

      bool isAbsolute() const
      {
         return isPathAbsolute;
      }
      
      void setAbsolute(bool isAbsolute)
      {
         isPathAbsolute = isAbsolute;
         isPathStrValid = false;
      }

      /**
       * Reserve vector capacity.
       */
      void reserve(size_t size)
      {
         this->pathElems.reserve(size);
      }
};
   

#endif /*PATHVEC_H_*/
