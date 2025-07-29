local test = require("tests/harness")

return section(",", function()
	it("converts normal arguments to a list of just that", function()
		test.assert("[0]", ",0")
		test.assert("[1]", ",1")
		test.assert("[1234]", ",1234")
		test.assert("[-1234]", ",~1234")

		test.assert("['']", ',""')
		test.assert("['hello']", ',"hello"')

		test.assert("[true]", ",TRUE")
		test.assert("[false]", ",FALSE")
		test.assert("[null]", ",NULL")
	end)

	it("also converts lists to just a list of that", function()
		test.assert("[[]]", ",@")
		test.assert("[[4]]", ",,4")
		test.assert("[[1,2,3]]", ",+@123")
	end)

	it("accepts blocks as the only operand", function()
		test.must(",BLOCK a")
	end)

	it("requires exactly one argument (argument count)", function()
		test.refute(",")
		test.must(", 1")
	end)
end)
