#pragma once
#include "types.hpp"
#include <string>
#include <unordered_set>
namespace IkigaiScript {
    using ModulePrivilegeFlags = uint8_t;

    // bitfield for privileges
    enum class ModulePrivilege : ModulePrivilegeFlags {
        allPrivilege = static_cast<ModulePrivilegeFlags>(-1),
        unrestricted = 0,
        localFolderRead = 1,
        localFolderWrite = 2,
        fileSystemRead = 4,
        fileSystemWrite = 8,
        localNetwork = 16,
        internet = 32,
        experimental = 64,
    };

    inline ModulePrivilegeFlags operator| (const ModulePrivilege ours, const ModulePrivilege other) {
        return static_cast<ModulePrivilegeFlags>(ours) | static_cast<ModulePrivilegeFlags>(other);
    }

    inline ModulePrivilegeFlags operator^ (const ModulePrivilege ours, const ModulePrivilege other) {
        return static_cast<ModulePrivilegeFlags>(ours) ^ static_cast<ModulePrivilegeFlags>(other);
    }

    inline ModulePrivilegeFlags operator& (const ModulePrivilege ours, const ModulePrivilege other) {
        return static_cast<ModulePrivilegeFlags>(ours) & static_cast<ModulePrivilegeFlags>(other);
    }

    inline ModulePrivilegeFlags operator| (const ModulePrivilege ours, const ModulePrivilegeFlags other) {
        return static_cast<ModulePrivilegeFlags>(ours) | other;
    }

    inline ModulePrivilegeFlags operator^ (const ModulePrivilege ours, const ModulePrivilegeFlags other) {
        return static_cast<ModulePrivilegeFlags>(ours) ^ other;
    }

    inline ModulePrivilegeFlags operator& (const ModulePrivilege ours, const ModulePrivilegeFlags other) {
        return static_cast<ModulePrivilegeFlags>(ours) & other;
    }

    inline bool shouldAllow(const ModulePrivilegeFlags allowPolicy, const ModulePrivilegeFlags requiredPermissions) {
        return 0 == ((!allowPolicy) & requiredPermissions);
    }

    struct Module {
        ModulePrivilegeFlags requiredPermissions;
        ScopePtr scope;
        std::string sourcePath;
        std::unordered_set<std::string> exports;
        bool isScriptModule = false;

        Module(ModulePrivilegeFlags f, ScopePtr s, std::string path = "", std::unordered_set<std::string> exp = {}, bool isScript = false)
            : requiredPermissions(f), scope(s), sourcePath(std::move(path)), exports(std::move(exp)), isScriptModule(isScript) {}
    };
}
