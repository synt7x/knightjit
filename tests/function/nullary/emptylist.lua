local test = require("tests/harness")

return section("@", function()
	it("is an empty list", function()
		test.assert("[]", "@")
	end)
end)
