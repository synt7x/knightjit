local test = require("tests/harness")

return section("TRUE", function()
	it("is true", function()
		test.assert("true", "TRUE")
	end)
end)
