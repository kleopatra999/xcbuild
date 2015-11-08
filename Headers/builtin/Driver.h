/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef __builtin_Driver_h
#define __builtin_Driver_h

#include <builtin/Base.h>

namespace builtin {

class Driver {
protected:
    Driver();
    ~Driver();

public:
    virtual std::string name() = 0;

public:
    virtual int run(std::vector<std::string> const &args) = 0;
};

}

#endif // !__builtin_Driver_h