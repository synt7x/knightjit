local test = require("tests/harness")

return section("WHILE", function()
	it("returns null", function()
		test.assert("null", "WHILE 0 0")
	end)

	it("will not eval the body if the condition is true", function()
		test.assert("12", "; WHILE FALSE (QUIT 1) : 12")
	end)

	it("will eval the body until the condition is false", function()
		test.assert("45", "; = i 0 ; = sum 0 ; WHILE (< i 10) ; = sum + sum i = i + i 1 : sum")
	end)

	it("will return NULL, regardless of the condition", function()
		test.assert("null", "WHILE FALSE 1234")
		test.assert("null", "; = i 0 : WHILE (< i 10) : = i + i 1")
	end)

	it("does not accept BLOCK values", function()
		test.refute("; = a 0 : WHILE (BLOCK a) 1")
		test.refute("WHILE (BLOCK QUIT 0) 1")
	end)

	it("requires exactly two arguments", function()
		test.refute("WHILE")
		test.refute("WHILE 0")
		test.must("WHILE 0 1")
	end)
end)
