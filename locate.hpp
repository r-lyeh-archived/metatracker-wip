#pragma once

#include <string>
#include <map>

namespace fonts
{
    std::map< std::string, std::string > list();
    std::string locate( const std::string &ttfname );
}