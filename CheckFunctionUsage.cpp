/*
 * c++check - c/c++ syntax checking
 * Copyright (C) 2007 Daniel Marjamäki
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/
 */


//---------------------------------------------------------------------------
#include "CheckFunctionUsage.h"
#include "tokenize.h"
#include <sstream>
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// FUNCTION USAGE - Check for unused functions etc
//---------------------------------------------------------------------------

CheckFunctionUsage::CheckFunctionUsage( ErrorLogger *errorLogger )
{
    _errorLogger = errorLogger;
    functions.clear();
}

CheckFunctionUsage::~CheckFunctionUsage()
{

}


void CheckFunctionUsage::parseTokens( const Tokenizer &tokenizer )
{
    // Function declarations..
    for ( const TOKEN *tok = tokenizer.tokens(); tok; tok = tok->next )
    {
        if ( tok->FileIndex != 0 )
            continue;

        const TOKEN *funcname = 0;

        if ( TOKEN::Match( tok, "%type% %var% (" )  )
            funcname = tok->at(1);
        else if ( TOKEN::Match(tok, "%type% * %var% (") )
            funcname = tok->at(2);
        else if ( TOKEN::Match(tok, "%type% :: %var% (") && !TOKEN::Match(tok, TOKEN::getstr(tok,2)) )
            funcname = tok->at(2);

        // Check that ") {" is found..
        for (const TOKEN *tok2 = funcname; tok2; tok2 = tok2->next)
        {
            if ( TOKEN::Match(tok2, ")") )
            {
                if ( ! TOKEN::Match(tok2, ") {") && ! TOKEN::Match(tok2, ") const {") )
                    funcname = NULL;
                break;
            }
        }

        if ( funcname )
        {
            FunctionUsage &func = functions[ funcname->str ];

            // No filename set yet..
            if (func.filename.empty())
                func.filename = tokenizer.getFiles()->at(0);

            // Multiple files => filename = "+"
            else if (func.filename != tokenizer.getFiles()->at(0))
            {
                func.filename = "+";
                func.usedOtherFile |= func.usedSameFile;
            }
        }
    }

    // Function usage..
    for ( const TOKEN *tok = tokenizer.tokens(); tok; tok = tok->next )
    {
        const TOKEN *funcname = 0;

        if ( TOKEN::Match( tok, "[;{}.,()[=+-/&|] %var% [(),;]" ) ||
             TOKEN::Match(tok, ":: %var% (") ||
             TOKEN::Match(tok, "|= %var% (") ||
             TOKEN::Match(tok, "&= %var% (") ||
             TOKEN::Match(tok, "&& %var% (") ||
             TOKEN::Match(tok, "|| %var% (") )
            funcname = tok->next;

        // funcname ( => Assert that the end paranthesis isn't followed by {
        if ( TOKEN::Match(funcname, "%var% (") )
        {
            int parlevel = 0;
            for ( const TOKEN *tok2 = tok; tok2; tok2 = tok2->next )
            {
                if (TOKEN::Match(tok2, "("))
                    ++parlevel;

                else if (TOKEN::Match(tok2, ")"))
                {
                    --parlevel;
                    if (parlevel == 0 && (TOKEN::Match(tok2, ") {") || TOKEN::Match(tok2, ") const")))
                        funcname = NULL;
                    if ( parlevel <= 0 )
                        break;
                }
            }
        }

        if ( funcname )
        {
            FunctionUsage &func = functions[ funcname->str ];

            if ( func.filename.empty() || func.filename == "+" )
                func.usedOtherFile = true;

            else
                func.usedSameFile = true;
        }
    }
}




void CheckFunctionUsage::check()
{
    for ( std::map<std::string, FunctionUsage>::const_iterator it = functions.begin(); it != functions.end(); ++it )
    {
        const FunctionUsage &func = it->second;
        if ( func.usedOtherFile || func.filename.empty() )
            continue;
        if ( ! func.usedSameFile )
        {
            std::ostringstream errmsg;
            errmsg << "The function '" << it->first << "' is never used.";
            _errorLogger->reportErr( errmsg.str() );
        }
        else if ( ! func.usedOtherFile )
        {
/*
            std::ostringstream errmsg;
            errmsg << "The function '" << it->first << "' is only used in the file it was declared in so it should have local linkage.";
            _errorLogger->reportErr( errmsg.str() );
*/
        }
    }
}



