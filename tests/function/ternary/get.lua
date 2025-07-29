local test = require("tests/harness")

return section("GET", function()
	describe("when the first argument is a string", function()
		it("returns a substring of the original string", function()
			test.assert("a", 'GET "abcd" 0 1')
			test.assert("bc", 'GET "abcd" 1 2')
			test.assert("cd", 'GET "abcd" 2 2')
			test.assert("", 'GET "abcd" 3 0')
			test.assert("", 'GET "" 0 0')
		end)

		-- Skipping exhaustive combinations for brevity

		it("converts its arguments to the correct types", function()
			test.assert("f", 'GET "foobar" NULL TRUE')
		end)
	end)

	describe("when the first argument is a list", function()
		it("returns a substring of the original list", function()
			test.assert("['a']", 'GET +@"abcd" 0 1')
			test.assert("['b','c']", 'GET +@"abcd" 1 2')
			test.assert("[3,4]", "GET +@1234 2 2")
			test.assert("[]", "GET +@1234 3 0")
			test.assert("[]", "GET @ 0 0")
		end)

		-- Skipping exhaustive combinations for brevity

		it("converts its arguments to the correct types", function()
			test.assert("['f']", 'GET +@"foobar" NULL TRUE')
		end)
	end)

	it("does not accept BLOCK values anywhere (strict types)", function()
		test.refute("GET (BLOCK QUIT 0) 0 0")
		test.refute("GET '0' (BLOCK QUIT 0) 0")
		test.refute("GET '0' 0 (BLOCK QUIT 0)")
		test.refute("; = a 3 : GET (BLOCK a) 0 0")
		test.refute("; = a 3 : GET '0' (BLOCK a) 0")
		test.refute("; = a 3 : GET '0' 0 (BLOCK a)")
	end)
end)
