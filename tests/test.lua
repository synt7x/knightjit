local spec = require("tests/spec")
local log = require("tests/log")

local harness = {}

-- Initialize harness counters
harness.passed = 0
harness.failed = 0
harness.todo = 0
harness.total = 0

-- Set the knight executable to be tested
-- from the command line argument.
harness.executable = arg[1]

if not harness.executable then
	log.error("No executable specified. Please provide the path to the knight executable.")
	return
end

-- Run all tests
for _, test in ipairs(spec.tests) do
	local ok, message = pcall(test, harness)

	if not ok then
		log.error("Panic during test: " .. message)
	end
end

-- Output information about the test run
log.completed(harness.passed, harness.failed, harness.todo, harness.total)
