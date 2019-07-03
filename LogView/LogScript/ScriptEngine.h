#pragma once
#include <Windows.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <LogLib/mstring.h>

class ScriptException {
public:
    ScriptException(const std::mstring &str) {
        mErrDesc = str;
    }

    std::mstring what() {
        return mErrDesc;
    }
private:
    std::mstring mErrDesc;;
};

struct LogKeyword {
    std::mstring mKeyword;
    size_t mKeywordStart;
    size_t mKeywordEnd;
    DWORD mColour;
};

struct LogFilterResult {
    std::mstring mContent;
    std::list<LogKeyword> mKeywordSet;
    bool mValid;

    LogFilterResult() {
        mValid = false;
    }

    void Clear() {
        mContent.clear();
        mKeywordSet.clear();
        mValid = false;
    }
};

class CScriptEngine {
    struct FilterRule {
        //KeyWord, and logic
        std::set<std::mstring> mInclude;
        std::set<std::mstring> mExclude;
    };

public:
    CScriptEngine();
    virtual ~CScriptEngine();
    bool Compile(const std::mstring &script);
    bool InputLog(const std::mstring &content, size_t initPos, LogFilterResult &result);

private:
    std::vector<FilterRule> SimpleCompile(const std::mstring &script) const;
    std::vector<FilterRule> CalAndResult(const std::vector<FilterRule> &a, const std::vector<FilterRule> &b) const;
    std::vector<FilterRule> CalOrResult(const std::vector<FilterRule> &a, const std::vector<FilterRule> &b) const;
    void ScriptCleanUp(std::mstring &script) const;
    void SetRuleColour();
    bool OnRuleFilter(const std::mstring &lineStr) const;
    void OnStrColour(const std::mstring &filterStr, LogFilterResult &result) const;
    void ClearCache();

private:
    std::vector<FilterRule> mRuleSet;
    std::map<std::mstring, DWORD> mRuleRgb;
    std::vector<DWORD> mColourSet;
    std::map<std::mstring, std::vector<FilterRule>> mVarSet;
    std::map<char, std::set<std::mstring>> mSearchIndex;
};