local test = require("tests/harness")

return section("whitespace", function()
	describe("whitespace needed after integers", function()
		it("is needed before other integers", function()
			for _, chr in ipairs({ "", " ", "\t" }) do
				for _, chr2 in ipairs({ " ", "\t" }) do
					test.assert("3", "+ 1" .. chr .. chr2 .. "2")
				end
			end
		end)
		it("is not needed before variables", function()
			test.assert("3", "; = a 2 : + 1a")
		end)
		it("is not needed before functions", function()
			test.assert("2", "+ 1LENGTH 1")
			test.assert("7", "+ 1* 2 3")
		end)
		it("is not needed before strings", function()
			test.assert("3", '+ 1"2"')
		end)
	end)

	describe("whitespace needed after variables", function()
		it("is needed before integers", function()
			for _, chr in ipairs({ "", " ", "\t" }) do
				for _, chr2 in ipairs({ " ", "\t" }) do
					test.assert("3", "; = a 1 : + a" .. chr .. chr2 .. "2")
				end
			end
		end)
		it("is needed before other variables", function()
			for _, chr in ipairs({ "", " ", "\t" }) do
				for _, chr2 in ipairs({ " ", "\t" }) do
					test.assert("3", "; = a 1 ; = b 2 : + a" .. chr .. chr2 .. "b")
				end
			end
		end)
		it("is not needed before functions", function()
			test.assert("3", "; = a 2 : + aLENGTH 1")
			test.assert("7", "; = a 1 : + a* 2 3")
		end)
		it("is not needed before strings", function()
			test.assert("3", '; = a 1 : + a"2"')
		end)
	end)

	describe("whitespace needed after functions", function()
		it("is not needed before integers", function()
			test.assert("1", "LENGTH1")
			test.assert("false", "!1")
		end)
		it("is not needed before variables", function()
			test.assert("1", "; = a 3 : LENGTHa")
			test.assert("-3", "; = a 3 : ~a")
		end)
		it("is needed between word functions", function()
			for _, chr in ipairs({ "", " ", "\t" }) do
				for _, chr2 in ipairs({ " ", "\t" }) do
					test.assert("0", "* RANDOM" .. chr .. chr2 .. 'LENGTH ""')
				end
			end
		end)
	end)
end)
