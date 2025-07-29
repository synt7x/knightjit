local test = require("tests/harness")

return section("RANDOM", function()
	it("should not return negative integers", function()
		test.assert("false", ";;=rF;=c 0W>10000=c+c 1=r|r>0Rr")
	end)

	it("should return a random value each time it is called", function()
		test.assert("false", "? RANDOM RANDOM")
	end)
end)
