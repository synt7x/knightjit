local test = require("tests/harness")

return section("variable", function()
	it("can be assigned to", function()
		test.assert("3", "; = a 3 : a")
	end)
	it("can be reassigned", function()
		test.assert("4", "; = a 3 ; = a 4 : a")
	end)
	it("can be reassigned using itself", function()
		test.assert("4", "; = a 3 ; = a + a 1 : a")
	end)
	it("can have multiple variables", function()
		test.assert("7", "; = a 3 ; = b 4 : + a b")
	end)
	it("has all variables as global within blocks", function()
		test.assert(
			"[5,2,6,4,7,8]",
			"; = a 1 ; = b 2 ; = blk BLOCK ; = a 5 ; = c 6 ; = e 7 ; = f 8 : ++++,a,b,c,d,e ; = c 3 ; = d 4 : +CALL blk ,f"
		)
	end)
end)
