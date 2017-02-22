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

#pragma once

#include <iostream>
#include <fstream>
#include <string>

// verify(expression) macro assumes appropriate Windows headers included elsewhere
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define verify(expression) ((expression) == false && (std::cerr << "ERROR: " << __FILENAME__ << ", line " << __LINE__ << ", function " << __func__ << std::endl, SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX), abort(), false))

class OstreamRedirector
{
public:
    // calling this on the same stream multiple times is undefined behavior
    void Redirect(std::ostream &os, const std::string &filename) {
        if (ofs.is_open() == true) ofs.close();
        ofs.open(filename, std::ios::trunc);
        verify(ofs.is_open() == true);
        pos = &os;
        psb = os.rdbuf(ofs.rdbuf()); // save and replace streambuf
        verify(psb != nullptr);
    }

    OstreamRedirector() : pos(nullptr), psb(nullptr) {}
    OstreamRedirector(std::ostream &os, const std::string &filename) { Redirect(os, filename); }
    ~OstreamRedirector() { if (pos != nullptr) pos->rdbuf(psb); } // restore streambuf
protected:
    std::ofstream   ofs;
    std::ostream   *pos;
    std::streambuf *psb;
};
