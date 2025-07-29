local test = require("tests/harness")

return section("FALSE", function()
	it("is false", function()
		test.assert("false", "FALSE")
	end)
end)
