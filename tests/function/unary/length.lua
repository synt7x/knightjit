local test = require("tests/harness")

return section("LENGTH", function()
	it("returns 0 for NULL", function()
		test.assert("0", "LENGTH NULL")
	end)

	it("returns the amount of digits in an integer", function()
		test.assert("1", "LENGTH 0")
		test.assert("1", "LENGTH 1")
		test.assert("2", "LENGTH 59")
		test.assert("4", "LENGTH 1111")
	end)

	it("returns the amount of chars in strings", function()
		test.assert("0", 'LENGTH ""')
		test.assert("3", 'LENGTH "foo"')
		test.assert("27", 'LENGTH "a man a plan a canal panama"')
		test.assert("21", 'LENGTH "and then I questioned"')
		test.assert(
			"100",
			"LENGTH 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'"
		)
	end)

	it("does not coerce its argument to an integer and back", function()
		test.assert("2", 'LENGTH "-0"')
		test.assert("5", 'LENGTH "49.12"')
		test.assert("1", 'LENGTH ,"49.12"')
	end)

	it("returns the amount of elements in a list", function()
		test.assert("0", "LENGTH @")
		test.assert("1", "LENGTH ,1")
		test.assert("3", "LENGTH +@123")
		test.assert("3", 'LENGTH +@"aaa"')
		test.assert("6", 'LENGTH + (+@"aaa") (+@"bbb") ')
		test.assert("100", "LENGTH *,33 100")
		test.assert("4", "LENGTH GET *,33 100 0 4")
	end)

	it("requires exactly one argument (argument count)", function()
		test.refute("LENGTH")
		test.must("LENGTH 1")
	end)
end)
