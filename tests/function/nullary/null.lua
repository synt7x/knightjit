local test = require("tests/harness")

return section("NULL", function()
	it("is null", function()
		test.assert("null", "NULL")
	end)
end)
