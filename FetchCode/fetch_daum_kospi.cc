/*
   Copyright 2015 Hosang Yoon

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <cassert>

int main(int argc, char *argv[])
{
    typedef std::string STR;
    std::set<STR>    setETF;    
    std::vector<STR> vecKOSPI;
    
    STR curlfile("curl.html");
    STR strETF("<td class=\"txt\"><a href=\"/item/main.daum?code=A");
    for (int page = 1; page <= 8; ++page)
    {
        STR cmd("curl -s -o " + curlfile + " \"http://finance.daum.net/quote/etf.daum?col=volume&order=desc&page=" + std::to_string(page) + "\"");
        system(cmd.c_str());
        std::ifstream ifs(curlfile);
        assert(ifs.is_open() == true);
        for (STR line; std::getline(ifs, line);)
        {
            auto pos = line.find(strETF);
            if (STR::npos != pos)
                setETF.insert(line.substr(pos + strETF.size(), 6));
        }
    }
    
    STR strKOSPI("<td class=\"txt\"><a href=\"/item/main.daum?code=");
    STR endKOSPI("</a></td>");
    for (int page = 1; page <= 16; ++page)
    {
        STR cmd("curl -s -o " + curlfile + " \"http://finance.daum.net/quote/marketvalue.daum?stype=P&page=" + std::to_string(page) + "&col=listprice&order=desc\"");
        system(cmd.c_str());
        std::ifstream ifs(curlfile);
        assert(ifs.is_open() == true);
        for (STR line; std::getline(ifs, line);)
        {
            auto pos = line.find(strKOSPI);
            if (STR::npos != pos)
            {
                STR code = line.substr(pos + strKOSPI.size(), 6);
                if (std::end(setETF) == setETF.find(code) && vecKOSPI.size() < 400) // skip ETFs and limit max number
                {
                    vecKOSPI.push_back(code);

                    auto posNameBeg = pos + strKOSPI.size() + 8;
                    auto posNameEnd = line.find(endKOSPI);
                    std::cout << '{' << code << '}' << ' ' << line.substr(posNameBeg, posNameEnd - posNameBeg) << '\n';
                }
            }
        }
    }
    
    system(("rm " + curlfile).c_str());
    
    if (vecKOSPI.size() > 0)
    {
        std::cout << vecKOSPI[0];
        for (std::size_t idx = 1, size = vecKOSPI.size(); idx < size; ++idx)
            std::cout << ';' << vecKOSPI[idx];
        std::cout << '\n';
    }
    
    return 0;
}
