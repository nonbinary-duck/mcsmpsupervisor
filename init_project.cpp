#include <filesystem>
#include <functional>
#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <random>
#include <regex>

#include "CLI11.hpp"

namespace fs = std::filesystem;


//
// CoLiRu with:
//     g++ init_project.cpp -std=c++23 -Os -o init && ./init --help
// 


/**
 * @brief The type used to represent a single path
 */
typedef fs::directory_entry                                                action_path_type;
/**
 * @brief The type used to represent all the paths
 */
typedef std::vector<action_path_type>                                       action_tree_type;
/**
 * @brief A function to make changes to a path
 */
typedef std::function<void(action_path_type &path, action_tree_type *tree)> action_func_type;


/**
 * @brief Validator if a string is a valid key for cmake
 * 
 * @note Expanded from https://cliutils.github.io/CLI11/book/chapters/validators.html
 */
struct ValidKeyValidator : public CLI::Validator
{
    ValidKeyValidator()
    {        
        name_ = "VALID_KEY";

        func_ = [](const std::string &str)
        {
            if (!(0 < str.length() && 60 >= str.length())) return std::string("Key must be greater than zero characters and ≤ 60 long");
            
            // Check if the first character is a number
            if (!(0x61 <= str[0] && 0x7a >= str[0])) return std::string("First character of key must be [a-z]");
            
            // Check if every character of the string is a number/letter/underscore or dash
            for (auto &&c : str)
            {
                // Check if the character is valid
                if (!(0x30 <= c && c <= 0x39) && !(0x61 <= c && c <= 0x7a) && c != 0x5f && c != 0x2d)
                    return std::string("All characters in key must match pattern of a-z, 0-9, - or _ where the first character is a-z");
            }
            return std::string();
        };
    }
};
const static ValidKeyValidator ValidKey;


/**
 * @brief Apply an action recursively to every file and directory in a cwd
 * 
 * @param cwd The origin
 * @param ignoreRules Which paths to ignore
 * @param action The action to take
 */
inline void action_on_path(
    const fs::path &cwd,
    const std::vector<std::regex> &ignoreRules,
    const action_func_type &action)
{
    auto treeIter = fs::recursive_directory_iterator(cwd);
    auto *tree = new std::vector<fs::directory_entry>();

    // Iterate over the path and store what we find
    for (auto &&path : treeIter)
    {
        // Check if the path is a directory, file or symlink
        if (!(path.is_directory() || path.is_regular_file() || path.is_symlink())) continue;
        
        // Make a copy of each entry
        tree->push_back(path);
    }
    
    for (auto &&path : *tree)
    {
        bool skipThisPath = false;

        // if (((std::string)path.path()).find() )
        // Loop over all ignore rules and ignore if it matches
        for (auto &&rule : ignoreRules)
        {
            // Check if we ignore this path
            if (std::regex_search(path.path().c_str(), rule))
            {
                skipThisPath = true;
                break;
            }
        }

        // Apply the ignore rules
        if (skipThisPath) continue;
        
        action(path, tree);
    }
    
    delete tree;
}


/**
 * @brief Called when the program is launched
 * 
 * @param argc Count of command-line arguments
 * @param argv Args, zero is the name of the program
 * @return int An error code
 */
int main(int argc, char *argv[])
{
    // Setup CLI11 stuff
    CLI::App app{"A simple project initialisation script, finalises by removing this binary and its source", "init_project"};
    
    std::string projName = "default";
    app.add_option("project_name", projName, "The name of the project")
        ->check(ValidKey)
        ->required()
        ->ignore_case();


    std::string execName = "default";
    app.add_option("exec_name", execName, "The name of the hello world default executable")
        ->check(ValidKey)
        ->required()
        ->ignore_case();


    ulong projNum        = -1;
    app.add_option("-n,--num", projNum, "Project number, assigned ND random 6-digit number by default")
        ->check(CLI::Range(0ul,999'999ul))
        ->ignore_case();
    

    bool dryRun          = false;
    app.add_flag("-d,--dry-run", dryRun, "Perform a dry-run execution without making changes")
        ->ignore_case();


    bool selfDestruct    = true;
    app.add_flag("!-q,!--no-self-destruct", selfDestruct, "Do not remove this binary and its source after operation")
        ->ignore_case()
        ->default_val(true);


    // Init app
    CLI11_PARSE(app, argc, argv);

    if (dryRun) selfDestruct = false;

    if (projNum == -1UL)
    {
        // Use a non-deterministic random value for the project ID if no ID was manually specified
        std::random_device rd;
        projNum = rd() % 1'000'000;
    }
    

    // Convert the project number into a 6-digit string
    std::string projNumStr("000000");
    for (size_t i = 0; i < projNumStr.length(); i++)
    {
        projNumStr[i] = std::to_string(( (ulong)( projNum / std::pow(10, 5-i) ) ) % 10)[0];
    }
    
    // Get the current working directory
    auto cwd = fs::current_path();

    // Get paths related to this application
    std::vector<std::filesystem::path> selfDestructPaths = {
        cwd / "init",
        cwd / "init_project.cpp",
        cwd / "CLI11.hpp"
    };

    // Check if there is a gitignore and .git directory (check we're in the right place)
    if (!fs::exists(cwd / ".git") | !fs::exists(cwd / ".gitignore"))
        throw std::runtime_error("Must be executed where CWD is a directory with git is set up with a gitignore file");


    // Read entries in the gitignore and treat each line as a regex rule
    std::ifstream gitignoreStream( cwd / ".gitignore" , std::ios::binary);
    // Start with some additional rules
    std::vector<std::regex> ignoreRules = {
        std::regex(".git"),
        std::regex(".gitignore"),
        std::regex("CLI11.hpp"),
        std::regex("init_project.cpp"),
        std::regex("init")
    };


    // It really should always open the file stream, since we know the file exists
    if (!gitignoreStream.is_open()) throw std::runtime_error("Failed to open .gitignore file");
    else
    {
        // Read the gitignore line-by-line
        for (std::string line; std::getline(gitignoreStream, line);)
        {
            // Ignore "rule" if the line is empty or if its a comment
            if      (line.empty())   continue;
            else if (line[0] == '#') continue;

            // Add this line as a rule
            ignoreRules.push_back( std::regex(line) );
        }   
    }
    

    // Define the patterns we want to replace
    std::vector<std::pair<std::regex, std::string>> replacePatterns = {
        { std::regex("__PROJID__"), projNumStr },
        { std::regex("<PROJ>"),     projName   },
        { std::regex("<EXEC>"),     execName   }
    };
    
    /**
     * @brief A function to replace text by pattern (captures used variables by reference)
     */
    auto replaceText = [&](action_path_type &path, action_tree_type *tree)->void
    {
        // No action if dir
        if (!path.is_regular_file()) return;
        
        // If we have at least one regex hits
        bool hit = false;

        // Reading taken from en.cppreference.com: https://en.cppreference.com/w/cpp/io/basic_istream/read
        
        // Immediately seek to the end of the file so we know the size of it
        // It needs to open
        if (std::fstream fIStream{path.path() , std::ios::binary | std::ios::ate | std::ios::in})
        {
            auto fSize = fIStream.tellg();
            // Return to the beginning of the file
            fIStream.seekg(0);
            std::string fBuffer(fSize, '\0');
            // Write into our buffer
            if (!fIStream.read(&fBuffer[0], fSize))
                std::cout << "Failed to read " << path << '\n';

            // Loop over regex rules
            for (auto &&rule : replacePatterns)
            {
                // Search for our rule
                if (std::regex_search(fBuffer, rule.first))
                {
                    // Register the hit
                    hit = true;
                    // We don't take any action on dry run (and only care if one rule hits)
                    if (dryRun) break;

                    // Replace
                    fBuffer = std::regex_replace(fBuffer, rule.first, rule.second);
                }
            }

            // Write out new (entire) file
            if (hit && !dryRun)
            {
                // We must close our reading stream
                fIStream.close();
                
                // We need a new instance of fstream
                if (std::fstream fOStream{path.path() , std::ios::binary | std::ios::ate | std::ios::out | std::ios::trunc})
                {
                    // Seek to beginning for write
                    fOStream.seekp(0);
                    // Size has now potentially changed if the replace is of different size to the rule matches
                    fOStream.write(&fBuffer[0], fBuffer.size());
                }
                else std::cout << "Failed to open for write " << path << '\n';
            }
            else if (hit)
                std::cout << "(dry run) Applied RegEx changes to " << path << '\n';

        }
        else
            std::cout << "Failed to open " << path << '\n';
        
    };
    
    /**
     * @brief A function to rename directories by pattern
     */
    auto moveDirs    = [&](action_path_type &path, action_tree_type *tree)->void
    {
        // Loop over the replace rules
        for (auto &&rule : replacePatterns)
        {
            // We could get multiple hits
            auto oldPath            = path.path();
            auto oldFname           = oldPath.filename();
            const auto oldFnameCstr = oldFname.c_str();

            if (std::regex_search(oldFnameCstr, rule.first))
            {
                // For the first match, apply and return
                auto newFname = std::regex_replace(oldFnameCstr, rule.first, rule.second);
                auto newPath  = (oldPath.parent_path() / newFname);

                // Apply action / don't
                if (!dryRun)
                {
                    fs::rename(oldPath, newPath);
                }
                else
                {
                    printf("  %s ⇢ %s\n", oldPath.c_str(), newPath.c_str());
                }

                // Update this path in memory
                path.assign(newPath);

                // If the path is a directory, then we'll need to update
                // subdirectories of that path in memory to match reality
                // In dry-run mode the is_directory() can return garbage
                // as we're making changes only in memory and not on disk
                if (path.is_directory() | dryRun)
                {
                    // A generic representation of the modified path
                    auto oldPathStr(oldPath.generic_string());
                    
                    for (auto &&tEnt : *tree)
                    {
                        const auto &tPath = tEnt.path();
                        
                        // Only modify derivative paths
                        if (tPath.generic_string().starts_with(oldPathStr))
                        {
                            // Modify the old path to be derivative of the new path
                            tEnt.assign(newPath / fs::relative(tPath, oldPath));
                        }
                    }
                }
            }
        }
    };

    if (dryRun)
    {
        // Inform user that we're in dry-run mode
        std::cout << "Running in dry-run mode:\n= Move operations    =\n";
    }

    // Move dirs
    action_on_path(cwd, ignoreRules, moveDirs);

    if (dryRun)
        std::cout << "\n= Replace operations =\n\n";

    // Replace text
    action_on_path(cwd, ignoreRules, replaceText);

    if (dryRun)
        std::cout << '\n';

    if (!dryRun && selfDestruct)
    {
        // Clean up after us
        for (auto &&path : selfDestructPaths)
        {
            std::filesystem::remove_all(path);
        }
    }

    // Summarise action
    printf("Initiated project with project id %s name %s and executable name %s\n", projNumStr.c_str(), projName.c_str(), execName.c_str());
    
    return 0;
}
