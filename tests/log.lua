local log = {}

log.colors = {
	red = "\27[31m",
	green = "\27[32m",
	yellow = "\27[33m",
	blue = "\27[34m",
	magenta = "\27[35m",
	cyan = "\27[36m",
	grey = "\27[90m",
	reset = "\27[0m",
}

function log.completed(passed, failed, todo, total)
	print(
		"Successfully ran tests with "
			.. log.colors.green
			.. passed
			.. log.colors.reset
			.. " passed, "
			.. log.colors.red
			.. failed
			.. log.colors.reset
			.. " failed, "
			.. log.colors.yellow
			.. todo
			.. log.colors.reset
			.. " todo, out of "
			.. log.colors.blue
			.. total
			.. log.colors.reset
			.. " tests."
	)
end



function log.it(section, description, name)
	print(
		"[" .. log.colors.magenta .. section .. log.colors.reset .. "] " .. (name and name .. " " or "") .. description
	)
end

function log.assert(pass, stdout, code, stdin, expected)
	io.write("  ")

	expected = expected:gsub("\n", "\\n"):gsub("\r", "\\r")
	stdout = stdout:gsub("\n", "\\n"):gsub("\r", "\\r")

	if pass then
		if not stdin then
			print(
				log.colors.green
					.. "PASS"
					.. log.colors.reset
					.. ": "
					.. code
					.. log.colors.grey
					.. " => "
					.. log.colors.reset
					.. stdout
			)
		else
			print(
				log.colors.green
					.. "PASS"
					.. log.colors.reset
					.. ": "
					.. stdin
					.. log.colors.grey
					.. " -> "
					.. log.colors.reset
					.. code
					.. log.colors.grey
					.. " => "
					.. log.colors.reset
					.. stdout
			)
		end
	else
		if not stdin then
			print(
				log.colors.red
					.. "FAIL"
					.. log.colors.reset
					.. ": "
					.. code
					.. log.colors.grey
					.. " => "
					.. log.colors.reset
					.. stdout
					.. " (expected: "
					.. expected
					.. ")"
			)
		else
			print(
				log.colors.red
					.. "PASS"
					.. log.colors.reset
					.. ": "
					.. stdin
					.. log.colors.grey
					.. " -> "
					.. log.colors.reset
					.. code
					.. log.colors.grey
					.. " => "
					.. log.colors.reset
					.. stdout
					.. " (expected: "
					.. expected
					.. ")"
			)
		end
	end
end

function log.todo(message)
	io.write("  ")
	print(log.colors.yellow .. "TODO" .. log.colors.reset .. ": " .. message)
end

function log.error(message)
	print(log.colors.red .. "ERROR" .. log.colors.reset .. ": " .. message)
end

return log
