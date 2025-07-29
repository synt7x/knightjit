local test = require("tests/harness")

return section("BLOCK", function()
	it("should not eval its argument", function()
		test.must("BLOCK variable")
		test.must("BLOCK QUIT 1")
		test.must("BLOCK + a 4")
	end)

	it("accepts blocks as its sole operand", function()
		test.must("BLOCK BLOCK QUIT 1")
	end)

	it("requires exactly one argument (argument count)", function()
		test.refute("BLOCK")
		test.must("BLOCK 1")
	end)
end)
