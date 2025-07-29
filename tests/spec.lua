local log = require("tests/log")

-- Test files should generally follow the structure of
-- ```
-- section("<name>", function()
--     it("<test description>", function()
--         test.assert("<stdout>", "<knight code>", "(stdin)")
--         ... more tests ...
--     end)
--
--     ... more tests ...
-- end)
-- ```
local spec = {}

-- Specify the paths to the test directories,
-- relative to the base 'tests' directory.
-- Inside the folder, there should be a 'spec.lua' file.
spec.scopes = {
	"function/nullary",
	"function/unary",
	"function/binary",
	"function/ternary",
	"function/quaternary",
	"syntax",
	"types",
	"variable",
}

-- Each test should be the result of a file returning
-- a single `section` with the `name` field set to what
-- is being tested.
spec.tests = {}

for _, scope in ipairs(spec.scopes) do
	local path = "tests/" .. scope .. "/spec"
	local ok, test = pcall(require, path)

	if not ok then
		log.error("Could not locate/require '" .. path .. "'")
	end

	if not test or type(test) ~= "function" then
		log.error("Test file '" .. path .. "' does not return a valid test function.")
		return spec
	end

	-- Add the test function to the tests table
	table.insert(spec.tests, test("tests/" .. scope))
end

return spec
