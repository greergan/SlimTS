#include <unordered_map>
#include <slim/path.h>
#if defined(_WIN32)
    std::string slim::path::getExecutablePath() {
        char rawPathName[MAX_PATH];
        GetModuleFileNameA(NULL, rawPathName, MAX_PATH);
        return std::string(rawPathName);
    }
    std::string slim::path::getExecutableDir() {
        std::string executablePath = getExecutablePath();
        char* exePath = new char[executablePath.length()];
        strcpy_s(exePath, executablePath.c_str());
        PathRemoveFileSpecA(exePath);
        std::string directory = std::string(exePath);
        delete[] exePath;
        return directory;
    }
    std::string slim::path::mergePaths(std::string pathA, std::string pathB) {
        char combined[MAX_PATH];
        PathCombineA(combined, pathA.c_str(), pathB.c_str());
        std::string mergedPath(combined);
        return mergedPath;
    }
#endif
#ifdef __linux__
    std::string slim::path::getExecutablePath() {
        char rawPathName[PATH_MAX];
        auto result = realpath(PROC_SELF_EXE, rawPathName);
        return std::string(rawPathName);
    }
    std::string slim::path::getExecutableDir() {
        std::string executablePath = getExecutablePath();
        char *executablePathStr = new char[executablePath.length() + 1];
        strcpy(executablePathStr, executablePath.c_str());
        std::string executableDir(dirname(executablePathStr));
        delete [] executablePathStr;
        return executableDir;
    }
    std::string slim::path::mergePaths(std::string pathA, std::string pathB) {
        return pathA+"/"+pathB;
    }
#endif
#ifdef __APPLE__
    std::string slim::path::getExecutablePath() {
        char rawPathName[PATH_MAX];
        char realPathName[PATH_MAX];
        uint32_t rawPathSize = (uint32_t)sizeof(rawPathName);
        if(!_NSGetExecutablePath(rawPathName, &rawPathSize)) {
            realpath(rawPathName, realPathName);
        }
        return std::string(realPathName);
    }
    std::string slim::path::getExecutableDir() {
        std::string executablePath = getExecutablePath();
        char *executablePathStr = new char[executablePath.length() + 1];
        strcpy(executablePathStr, executablePath.c_str());
        char* executableDir = dirname(executablePathStr);
        delete [] executablePathStr;
        return std::string(executableDir);
    }
    std::string slim::path::mergePaths(std::string pathA, std::string pathB) {
        return pathA+"/"+pathB;
    }
#endif
bool slim::path::checkIfFileExists(const std::string& filePath) {
	return access(filePath.c_str(), 0 ) == 0;
}