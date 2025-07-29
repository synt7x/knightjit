local test = require("tests/harness")

return section("ASCII", function()
	it("converts integers properly", function()
		for _, char in ipairs({ "A", "B", "C", "a", "b", "c", "0", "1", "2", "@", " ", "!" }) do
			test.assert(char, "ASCII " .. string.byte(char))
		end
	end)

	it("converts strings properly", function()
		test.assert(tostring(string.byte('"')), "ASCII '\"'")
		for _, char in ipairs({ "A", "B", "C", "a", "b", "c", "0", "1", "2", "@", " ", "!" }) do
			test.assert(tostring(string.byte(char)), "ASCII '" .. char .. "'")
		end
	end)

	it("converts multicharacter strings properly", function()
		test.assert(tostring(string.byte("H")), "ASCII 'HELLO'")
		test.assert(tostring(string.byte("n")), "ASCII 'neighbor'")
	end)

	it("only allows an integer or string as the first operand", function()
		test.refute("ASCII TRUE")
		test.refute("ASCII FALSE")
		test.refute("ASCII NULL")
		test.refute("ASCII @")
	end)

	it("does not allow a block", function()
		test.refute("; = a 3 : ASCII (BLOCK a)")
		test.refute("; = a (BLOCK QUIT 0) : ASCII a")
	end)

	it("requires exactly one argument", function()
		test.refute("ASCII")
		test.must("ASCII 'a'")
	end)
end)
