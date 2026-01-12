#pragma once
#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_BAJE_PARSER
    #define BAJE_PARSER_PUBLIC __declspec(dllexport)
  #else
    #define BAJE_PARSER_PUBLIC __declspec(dllimport)
  #endif
#else
  #ifdef BUILDING_BAJE_PARSER
      #define BAJE_PARSER_PUBLIC __attribute__ ((visibility ("default")))
  #else
      #define BAJE_PARSER_PUBLIC
  #endif
#endif

namespace baje_parser {

class BAJE_PARSER_PUBLIC Baje_parser {

public:
  Baje_parser();
  int get_number() const;

private:

  int number;

};

}

