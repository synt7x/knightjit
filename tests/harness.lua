local log = require("tests/log")

-- Get the null device path based on the OS.
function null()
	if package.config:sub(1, 1) == "\\" then
		return "NUL"
	else
		return "/dev/null"
	end
end

-- Create a test section, generally
-- it is best practice to use the name
-- of the operation being tested.
function section(name, callback)
	if type(name) ~= "string" or name == "" then
		error("Section name must be a non-empty string.")
		return
	end

	if type(callback) ~= "function" then
		error("Section for '" .. name .. "' callback must be a function.")
		return
	end

	-- Expose section name to be used in `it` calls
	section_name = name

	-- Run the user-defined test
	return function(harness)
		test_harness = harness or {}
		callback()
	end
end

-- Describe a series of test cases that
-- are related within the section.
function describe(description, callback)
	if type(description) ~= "string" or description == "" then
		error("Description must be a non-empty string.")
		return
	end

	if type(callback) ~= "function" then
		error("Describe for '" .. description .. "' callback must be a function.")
		return
	end

	-- Expose the description to be used in `it` calls
	description_name = description

	-- Run the user-defined test
	callback()

	-- Restore the description name
	description_name = nil
end

-- Create a test case within a section.
-- Should contain multiple assertions to
-- fulfill the description.
function it(description, callback)
	if type(description) ~= "string" or description == "" then
		error("Test description must be a non-empty string.")
		return
	end

	if type(callback) ~= "function" then
		error("Test for '" .. description .. "' callback must be a function.")
		return
	end

	log.it(section_name, description, description_name)

	-- Run the user-defined test
	callback()

	if arg[2] and test_harness.failed > 0 then
		log.error("Tests failed. Exiting early due to stop flag.")
		os.exit(1)
	end
end

-- Utility functions for the test harness
local test = {}

-- Expose a test spec to be used by the harness.
function test.spec(specs)
	if type(specs) ~= "table" then
		error("Test specs must be a table.")
		return
	end

	return function(path)
		return function(harness)
			for _, spec in ipairs(specs) do
				local path = path .. "/" .. spec
				local ok, test = pcall(require, path)

				if not ok then
					log.error("Failed to load test file '" .. path .. "': " .. test)
					return
				end

				if not test or type(test) ~= "function" then
					log.error("Test file '" .. path .. "' does not return a valid test function.")
					return
				end

				-- Run the test function with the harness
				test(harness)
			end
		end
	end
end

-- Run a piece of Knight code and return the
-- output and exit code.
function test.run(code, stdin)
	code = code:gsub('"', '\\"')
	local echo = stdin and "echo " .. stdin .. " |" or ""

	local handle = io.popen(echo .. test_harness.executable .. ' -e "D ' .. code .. '"', "r")
	local stdout = handle:read("*a")
	local _, _, exit = handle:close()
	return stdout, exit
end

-- Refute the successful execution of a piece of Knight code.
function test.refute(code, stdin)
	code = code:gsub('"', '\\"')
	local echo = stdin and 'echo "' .. stdin .. '" |' or ""

	local handle = io.popen(echo .. test_harness.executable .. ' -e "' .. code .. '" 2>' .. null(), "r")
	local stdout = handle:read("*a")
	local _, _, exit = handle:close()
	local pass = exit ~= 0

	local message = pass and log.colors.red .. "[PANIC]" .. log.colors.reset
		or log.colors.green .. "[SUCCESS]" .. log.colors.reset

	log.assert(pass, message, code, stdin, "Expected panic")

	if pass then
		test_harness.passed = test_harness.passed + 1
	else
		test_harness.failed = test_harness.failed + 1
	end

	test_harness.total = test_harness.total + 1
end

-- Require the successful execution of a piece of Knight code.
function test.must(code, stdin)
	code = code:gsub('"', '\\"')
	local echo = stdin and 'echo "' .. stdin .. '" |' or ""

	local handle = io.popen(echo .. test_harness.executable .. ' -e "' .. code .. '"', "r")
	local stdout = handle:read("*a")
	local _, _, exit = handle:close()
	local pass = exit == 0

	local message = pass and log.colors.green .. "[SUCCESS]" .. log.colors.reset
		or log.colors.red .. "[PANIC]" .. log.colors.reset

	log.assert(pass, message, code, stdin, "Expected success")

	if pass then
		test_harness.passed = test_harness.passed + 1
	else
		test_harness.failed = test_harness.failed + 1
	end

	test_harness.total = test_harness.total + 1
end

-- Assert that the output of the Knight code
-- matches the expected output.
function test.assert(stdout, code, stdin)
	local output, result = test.run(code, stdin)
	output = output:gsub("\n$", "")
	local pass = output == stdout

	log.assert(pass, output, code, stdin, stdout)

	if pass then
		test_harness.passed = test_harness.passed + 1
	else
		test_harness.failed = test_harness.failed + 1
	end

	test_harness.total = test_harness.total + 1
end

-- Mark a test as TODO.
function test.todo(message)
	log.todo(message)
	test_harness.total = test_harness.total + 1
	test_harness.todo = test_harness.todo + 1
end

return test
