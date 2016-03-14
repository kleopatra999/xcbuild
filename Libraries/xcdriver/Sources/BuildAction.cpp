/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <xcdriver/BuildAction.h>
#include <xcdriver/Action.h>
#include <xcdriver/Options.h>
#include <xcexecution/NinjaExecutor.h>
#include <xcexecution/SimpleExecutor.h>
#include <xcformatter/DefaultFormatter.h>
#include <builtin/builtin.h>
#include <libutil/Base.h>

#include <unistd.h>

using xcdriver::BuildAction;
using xcdriver::Options;

BuildAction::
BuildAction()
{
}

BuildAction::
~BuildAction()
{
}

static std::shared_ptr<xcformatter::Formatter>
CreateFormatter(std::string const &formatter)
{
    if (formatter == "default" || formatter.empty()) {
        /* Only use color if attached to a terminal. */
        bool color = isatty(fileno(stdout));

        auto formatter = xcformatter::DefaultFormatter::Create(color);
        return std::static_pointer_cast<xcformatter::Formatter>(formatter);
    }

    return nullptr;
}

static std::unique_ptr<xcexecution::Executor>
CreateExecutor(
    std::string const &executor,
    std::shared_ptr<xcformatter::Formatter> const &formatter,
    bool dryRun,
    bool generate)
{
    if (executor == "simple" || executor.empty()) {
        auto registry = builtin::Registry::Default();
        auto executor = xcexecution::SimpleExecutor::Create(formatter, dryRun, registry);
        return libutil::static_unique_pointer_cast<xcexecution::Executor>(std::move(executor));
    } else if (executor == "ninja") {
        auto executor = xcexecution::NinjaExecutor::Create(formatter, dryRun, generate);
        return libutil::static_unique_pointer_cast<xcexecution::Executor>(std::move(executor));
    }

    return nullptr;
}

static bool
VerifySupportedOptions(Options const &options)
{
    if (!options.toolchain().empty()) {
        fprintf(stderr, "warning: toolchain option not implemented\n");
    }

    if (!options.destination().empty() || !options.destinationTimeout().empty()) {
        fprintf(stderr, "warning: destination option not implemented\n");
    }

    if (options.parallelizeTargets() || options.jobs() > 0) {
        fprintf(stderr, "warning: job control option not implemented\n");
    }

    if (options.hideShellScriptEnvironment()) {
        fprintf(stderr, "warning: output control option not implemented\n");
    }

    if (options.enableAddressSanitizer() || options.enableCodeCoverage()) {
        fprintf(stderr, "warning: build mode option not implemented\n");
    }

    if (!options.derivedDataPath().empty()) {
        fprintf(stderr, "warning: custom derived data path not implemented\n");
    }

    if (!options.resultBundlePath().empty()) {
        fprintf(stderr, "warning: result bundle path not implemented\n");
    }

    return true;
}

int BuildAction::
Run(Options const &options)
{
    // TODO(grp): Implement these options.
    if (!VerifySupportedOptions(options)) {
        return -1;
    }

    /* Verify the build options are not conflicting or invalid. */
    if (!Action::VerifyBuildActions(options.actions())) {
        return -1;
    }

    /*
     * Create the formatter to format the build log.
     */
    std::shared_ptr<xcformatter::Formatter> formatter = CreateFormatter(options.formatter());
    if (formatter == nullptr) {
        fprintf(stderr, "error: unknown formatter %s\n", options.formatter().c_str());
        return -1;
    }

    /*
     * Create the executor used to perform the build.
     */
    std::unique_ptr<xcexecution::Executor> executor = CreateExecutor(options.executor(), formatter, options.dryRun(), options.generate());
    if (executor == nullptr) {
        fprintf(stderr, "error: unknown executor %s\n", options.executor().c_str());
        return -1;
    }

    /*
     * Use the default build environment. We don't need anything custom here.
     */
    ext::optional<pbxbuild::Build::Environment> buildEnvironment = pbxbuild::Build::Environment::Default();
    if (!buildEnvironment) {
        fprintf(stderr, "error: couldn't create build environment\n");
        return -1;
    }

    /* The build settings passed in on the command line override all others. */
    std::vector<pbxsetting::Level> overrideLevels = Action::CreateOverrideLevels(options, buildEnvironment->baseEnvironment());

    /*
     * Create the build parameters. The executor uses this to load a workspace and create a
     * build context, but is not required to when the parameters haven't changed from a cache.
     */
    xcexecution::Parameters parameters = Action::CreateParameters(options, overrideLevels);

    /*
     * Perform the build!
     */
    bool success = executor->build(*buildEnvironment, parameters);
    if (!success) {
        return 1;
    }

    return 0;
}