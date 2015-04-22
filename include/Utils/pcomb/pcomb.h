#ifndef PCOMB_MAIN_HEADER_H
#define PCOMB_MAIN_HEADER_H

// This is a header that pulls in all the headers for parsers and combinators
#include "Utils/pcomb/Parser/PredicateCharParser.h"
#include "Utils/pcomb/Parser/RegexParser.h"
#include "Utils/pcomb/Parser/StringParser.h"

#include "Utils/pcomb/Combinator/AltParser.h"
#include "Utils/pcomb/Combinator/SeqParser.h"
#include "Utils/pcomb/Combinator/ManyParser.h"
#include "Utils/pcomb/Combinator/TokenParser.h"
#include "Utils/pcomb/Combinator/ParserAdapter.h"
#include "Utils/pcomb/Combinator/LazyParser.h"

#endif
