#ifndef ENTRYID_H_
#define ENTRYID_H_

#include <common/storage/Metadata.h>

#include <algorithm>
#include <cstdio>
#include <exception>
#include <string>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

namespace db {

struct EntryID {
   unsigned sequence;
   unsigned timestamp;
   unsigned nodeID;

   // required for queries, since they use boost::tuple to avoid writing out huge comparison
   // operators
   EntryID()
      : sequence(0), timestamp(0), nodeID(0)
   {}

   EntryID(unsigned sequence, unsigned timestamp, unsigned nodeID)
      : sequence(sequence), timestamp(timestamp), nodeID(nodeID)
   {
      if(nodeID == 0 && (timestamp != 0 || sequence > 2) )
         throw std::exception();
   }

   bool isSpecial() const { return nodeID == 0; }

   bool operator<(const EntryID& other) const
   {
      return boost::make_tuple(sequence, timestamp, nodeID)
         < boost::make_tuple(other.sequence, other.timestamp, other.nodeID);
   }

   bool operator==(const EntryID& other) const
   {
      return this->sequence == other.sequence
         && this->timestamp == other.timestamp
         && this->nodeID == other.nodeID;
   }

   bool operator!=(const EntryID& other) const
   {
      return !(*this == other);
   }

   std::string str() const
   {
      if(isSpecial() )
      {
         if(*this == anchor() )
            return "";

         if(*this == root() )
            return META_ROOTDIR_ID_STR;

         if(*this == disposal() )
            return META_DISPOSALDIR_ID_STR;

         throw std::exception();
      }

      char buffer[3*8 + 2 + 1];
      sprintf(buffer, "%X-%X-%X", sequence, timestamp, nodeID);
      return buffer;
   }

   static EntryID anchor() { return EntryID(0, 0, 0); }
   static EntryID root() { return EntryID(1, 0, 0); }
   static EntryID disposal() { return EntryID(2, 0, 0); }

   static EntryID fromStr(const std::string& str)
   {
      std::pair<bool, EntryID> pair = tryFromStr(str);
      if (!pair.first)
         throw std::exception();

      return pair.second;
   }

   static std::pair<bool, EntryID> tryFromStr(const std::string& str)
   {
      if(str.empty() )
         return std::make_pair(true, anchor());

      if(str == META_ROOTDIR_ID_STR)
         return std::make_pair(true, root());

      if(str == META_DISPOSALDIR_ID_STR)
         return std::make_pair(true, disposal());

      std::string::size_type sep1;
      std::string::size_type sep2;

      sep1 = str.find('-');
      sep2 = str.find('-', sep1 + 1);

      if(sep1 == str.npos || sep2 == str.npos)
         return std::make_pair(false, EntryID());

      std::string seqStr = str.substr(0, sep1);
      std::string tsStr = str.substr(sep1 + 1, sep2 - (sep1 + 1));
      std::string nodeStr = str.substr(sep2 + 1);

      if(seqStr.empty() || tsStr.empty() || nodeStr.empty() ||
            seqStr.size() > 8 || tsStr.size() > 8 || nodeStr.size() > 8)
         return std::make_pair(false, EntryID());

      struct {
         static bool notHex(char c)
         {
            return !std::isxdigit(c);
         }

         bool operator()(const std::string& str) const
         {
            return std::count_if(str.begin(), str.end(), notHex) == 0;
         }
      } onlyHex;

      if(!onlyHex(seqStr) || !onlyHex(tsStr) || !onlyHex(nodeStr) )
         return std::make_pair(false, EntryID());

      unsigned seq;
      unsigned ts;
      unsigned node;

      std::sscanf(seqStr.c_str(), "%x", &seq);
      std::sscanf(tsStr.c_str(), "%x", &ts);
      std::sscanf(nodeStr.c_str(), "%x", &node);

      return std::make_pair(true, EntryID(seq, ts, node));
   }
};

}

#endif
