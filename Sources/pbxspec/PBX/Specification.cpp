/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <pbxspec/PBX/Specification.h>
#include <pbxspec/PBX/BuildPhase.h>
#include <pbxspec/PBX/BuildSystem.h>
#include <pbxspec/PBX/Compiler.h>
#include <pbxspec/PBX/FileType.h>
#include <pbxspec/PBX/Linker.h>
#include <pbxspec/PBX/PackageType.h>
#include <pbxspec/PBX/ProductType.h>
#include <pbxspec/PBX/PropertyConditionFlavor.h>
#include <pbxspec/PBX/Tool.h>
#include <pbxspec/Context.h>
#include <pbxspec/Manager.h>

using pbxspec::PBX::Specification;
using pbxspec::Manager;
using libutil::FSUtil;

Specification::Specification() :
    _isGlobalDomainInUI(false)
{
}

bool Specification::
inherit(Specification::shared_ptr const &base)
{
    if (base == nullptr || base.get() == this)
        return false;

    _base               = base;
    _clazz              = base->clazz();
    _isGlobalDomainInUI = base->isGlobalDomainInUI();
    _name               = base->name();
    _description        = base->description();
    _vendor             = base->vendor();
    _version            = base->version();

    return true;
}

bool Specification::
parse(Context *context, plist::Dictionary const *dict, std::unordered_set<std::string> *seen, bool check)
{
    auto unpack = plist::Keys::Unpack("Specification", dict, seen);

    auto T  = unpack.cast <plist::String> ("Type");
    auto C  = unpack.cast <plist::String> ("Class");
    auto I  = unpack.cast <plist::String> ("Identifier");
    auto BO = unpack.cast <plist::String> ("BasedOn");
    auto DO = unpack.cast <plist::String> ("Domain");
    auto GD = unpack.coerce <plist::Boolean> ("IsGlobalDomainInUI");
    auto N  = unpack.cast <plist::String> ("Name");
    auto D  = unpack.cast <plist::String> ("Description");
    auto V1 = unpack.cast <plist::String> ("Vendor");
    auto V2 = unpack.cast <plist::String> ("Version");

    if (!unpack.complete(check)) {
        fprintf(stderr, "%s", unpack.errors().c_str());
    }

    if (T != nullptr) {
        /* Nothing to do. (If this is used, see `Context`'s `defaultType`. */
    }

    if (C != nullptr) {
        _clazz = C->value();
    }

    if (DO != nullptr) {
        _domain = DO->value();
    } else {
        _domain = context->domain;
    }

    if (I != nullptr) {
        _identifier = I->value();
    }

    if (BO != nullptr) {
        auto basedOn           = BO->value();

        auto basedOnDomain     = Manager::AnyDomain();
        auto basedOnIdentifier = basedOn;

        auto colon = basedOn.find(':');
        if (colon != std::string::npos) {
            basedOnDomain     = basedOn.substr(0, colon);
            basedOnIdentifier = basedOn.substr(colon + 1);
        }

        auto base = context->manager->specification(type(), basedOnIdentifier, { basedOnDomain });
        if (base == nullptr) {
            fprintf(stderr, "error: cannot find base %s specification '%s:%s'\n",
                    type(), basedOnDomain.c_str(), basedOnIdentifier.c_str());
            return false;
        }

        //
        // Inherit all the values.
        //
        if (!inherit(base))
            return false;
    }

    if (GD != nullptr) {
        _isGlobalDomainInUI = GD->value();
    }

    if (N != nullptr) {
        _name = N->value();
    }

    if (D != nullptr) {
        _description = D->value();
    }

    if (V1 != nullptr) {
        _vendor = V1->value();
    }

    if (V2 != nullptr) {
        _version = V2->value();
    }

    return true;
}

bool Specification::
ParseType(Context *context, plist::Dictionary const *dict, std::string const &expectedType, std::string *determinedType)
{
    std::string type;

    auto T = dict->value <plist::String> ("Type");
    if (T == nullptr) {
        if (!context->defaultType.empty()) {
            type = context->defaultType;
        } else {
            return false;
        }
    } else {
        type = T->value();
    }

    if (determinedType != nullptr) {
        *determinedType = type;
    }

    return (type == expectedType);
}

Specification::shared_ptr Specification::
Parse(Context *context, plist::Dictionary const *dict)
{
    std::string type;
    ParseType(context, dict, std::string(), &type);
    if (type.empty()) {
        fprintf(stderr, "error: specification missing type\n");
        return nullptr;
    }

    if (type == Architecture::Type())
        return Architecture::Parse(context, dict);
    if (type == BuildPhase::Type())
        return BuildPhase::Parse(context, dict);
    if (type == BuildSettings::Type())
        return BuildSettings::Parse(context, dict);
    if (type == BuildStep::Type())
        return BuildStep::Parse(context, dict);
    if (type == BuildSystem::Type())
        return BuildSystem::Parse(context, dict);
    if (type == Compiler::Type())
        return Compiler::Parse(context, dict);
    if (type == FileType::Type())
        return FileType::Parse(context, dict);
    if (type == Linker::Type())
        return Linker::Parse(context, dict);
    if (type == PackageType::Type())
        return PackageType::Parse(context, dict);
    if (type == ProductType::Type())
        return ProductType::Parse(context, dict);
    if (type == PropertyConditionFlavor::Type())
        return PropertyConditionFlavor::Parse(context, dict);
    if (type == Tool::Type())
        return Tool::Parse(context, dict);

    fprintf(stderr, "error: specification type '%s' not supported\n", type.c_str());

    return nullptr;
}

bool Specification::
Open(Context *context, std::string const &filename)
{
    if (filename.empty()) {
        fprintf(stderr, "stderr: empty specification path\n");
        return false;
    }

    std::string realPath = FSUtil::ResolvePath(filename);
    if (realPath.empty()) {
        fprintf(stderr, "stderr: invalid specification path\n");
        return false;
    }

    //
    // Parse property list
    //
    std::unique_ptr<plist::Object> plist = plist::Format::Any::Read(filename).first;
    if (plist == nullptr) {
        fprintf(stderr, "stderr: unable to read specification plist\n");
        return false;
    }

    //
    // If this is a dictionary, then it's a single specification,
    // if it's an array then multiple specifications are present.
    //
    if (auto dict = plist::CastTo <plist::Dictionary> (plist.get())) {
        auto spec = Parse(context, dict);
        if (spec != nullptr) {
            //
            // Add specification to manager.
            //
            context->manager->addSpecification(spec);
        }

        return (spec != nullptr);
    } else if (auto array = plist::CastTo <plist::Array> (plist.get())) {
        size_t errors = 0;
        size_t count  = array->count();
        for (size_t n = 0; n < count; n++) {
            if (auto dict = array->value <plist::Dictionary> (n)) {
                auto spec = Parse(context, dict);
                if (spec == nullptr) {
                    errors++;
                } else {
                    context->manager->addSpecification(spec);
                }
            } else {
                fprintf(stderr, "error: specification entry was not a dictionary\n");
                errors++;
            }
        }


        return (errors < count);
    }

    fprintf(stderr, "error: specification file '%s' does not contain a dictionary nor an array", filename.c_str());
    return false;
}

