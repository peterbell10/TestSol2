#define SOL_CHECK_ARGUMENTS 1
#include "sol.hpp"





struct Class
{
	enum Options
	{
		opt1,
		opt2
	};

	int mValueI;
	double mValueD;

	int fn(float aParam)
	{
		mValueD += aParam;
		return mValueI;
	}
};





/** Generic case: don't collect anything. */
template <typename T, typename... Args, std::enable_if_t<!std::is_member_object_pointer<T>::value, int> = 0>
std::vector<std::string> collectVariableNames(const char * aSymbolName, T & aSymbol, Args &... aArgs)
{
	return collectVariableNames(aArgs ...);
}





/** Terminator case: start with an empty vector. */
std::vector<std::string> collectVariableNames()
{
	return {};
}





/** If the next symbol is a member variable, add it to the returned strings. */
template <typename Member, typename... Args, std::enable_if_t<std::is_member_object_pointer<Member>::value, int> = 0>
std::vector<std::string> collectVariableNames(const char * aSymbolName, Member & aVariable, Args &... aArgs)
{
	auto res = collectVariableNames(aArgs...);
	res.push_back(aSymbolName);
	return res;
}






template <typename Class, typename... Args>
void registerClass(sol::state & aLua, const char * aClassName, Args &&... aArgs)
{
	auto variableNames = collectVariableNames(aArgs ...);
	aLua.new_usertype<Class>(aClassName, std::forward<Args>(aArgs)...);
	aLua[aClassName]["__vars"].set(sol::as_table(variableNames));
}





int main()
{
	using namespace sol;
	state lua;
	lua.open_libraries(
		lib::base,
		lib::package,
		lib::coroutine,
		lib::string,
		lib::os,
		lib::math,
		lib::table,
		lib::debug,
		lib::io
	);
	registerClass<Class>(lua, "Class",
		"mValueI", &Class::mValueI,
		"mValueD", &Class::mValueD,
		"fn", &Class::fn,
		"opt1", sol::var(Class::opt1),
		"opt2", sol::var(Class::opt2)
	);
	lua.set("cls", new Class());

	lua.script(
R"(
local function dumpTable(aTable, aName, aIndent)
	aIndent = aIndent or ""

	for i, v in pairs(aTable) do
		print(aIndent .. aName .. "." .. i .. ": (" .. type(v) .. ") " .. tostring(v));

		if (type(v) == "table") then
			dumpTable(v, aName .. "." .. i, aIndent .. "\t")
		end
	end
end


print("Class:")
dumpTable(Class, "Class")
print("\n\nmeta(Class):")
dumpTable(getmetatable(Class), "meta(Class) ")
print("\n\nmeta(cls):")
dumpTable(getmetatable(cls), "meta(cls) ")

)");

	lua.script("cls:fn(3)");
	lua.script("cls.mValueI = 10");
	return 0;
}
