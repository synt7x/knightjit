local test = require("tests/harness")

return section("THEN", function()
	it("executes arguments in order", function()
		test.assert("3", "; (= a 3) a")
	end)

	it("returns the second argument", function()
		test.assert("1", "; 0 1")
	end)

	it("also works with BLOCK return values", function()
		test.assert("3", "CALL ; = a 3 BLOCK a")
	end)

	it("accepts blocks as either argument", function()
		test.assert("3", "; (BLOCK QUIT 1) 3")
		test.assert("4", "CALL ; 3 (BLOCK 4)")
	end)

	it("requires exactly two arguments", function()
		test.refute(";")
		test.refute("; 1")
		test.must("; 1 1")
	end)
end)
