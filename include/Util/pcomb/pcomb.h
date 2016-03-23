#ifndef PCOMB_MAIN_HEADER_H
#define PCOMB_MAIN_HEADER_H

// This is a header that pulls in all the headers for parsers and combinators
#include "Util/pcomb/Parser/PredicateCharParser.h"
#include "Util/pcomb/Parser/RegexParser.h"
#include "Util/pcomb/Parser/StringParser.h"

#include "Util/pcomb/Combinator/AltParser.h"
#include "Util/pcomb/Combinator/SeqParser.h"
#include "Util/pcomb/Combinator/ManyParser.h"
#include "Util/pcomb/Combinator/TokenParser.h"
#include "Util/pcomb/Combinator/ParserAdapter.h"
#include "Util/pcomb/Combinator/LazyParser.h"
#include "Util/pcomb/Combinator/LexemeParser.h"

#endif
