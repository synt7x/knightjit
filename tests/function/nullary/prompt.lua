local test = require("tests/harness")

return section("PROMPT", function()
	it("should read a line from stdin, and strip `\\n`", function()
		test.assert("foo", "PROMPT", "foo\n")
		test.assert("foo", "PROMPT", "foo\nbar")
		test.assert("foo", "PROMPT", "foo\nbar\nbaz")
	end)

	it("should strip `\\r\\n`s as well", function()
		test.assert("foo", "PROMPT", "foo\r\n")
		test.assert("foo", "PROMPT", "foo\r\nbar")
	end)
end)
