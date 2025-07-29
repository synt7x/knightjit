local test = require("tests/harness")

return section("CALL", function()
	it("should run something returned by BLOCK", function()
		test.assert("12", "CALL BLOCK 12")
		test.assert("12", 'CALL BLOCK "12"')

		test.assert("true", "CALL BLOCK TRUE")
		test.assert("false", "CALL BLOCK FALSE")
		test.assert("null", "CALL BLOCK NULL")
		test.assert("[]", "CALL BLOCK @")

		test.assert("twelve", '; = foo BLOCK bar ; = bar "twelve" : CALL foo')
		test.assert("15", "; = foo BLOCK * x 5 ; = x 3 : CALL foo")
	end)

	it("should only eval blocks (strict compliance)", function()
		test.refute("CALL 1")
		test.refute('CALL "1"')
		test.refute("CALL TRUE")
		test.refute("CALL FALSE")
		test.refute("CALL NULL")
	end)

	it("requires exactly one argument (argument count)", function()
		test.refute("CALL")
		test.must("CALL BLOCK + 1 2")
	end)
end)
