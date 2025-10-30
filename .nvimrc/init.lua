vim.keymap.set("n", "<M-m>", function()
	-- vim.cmd("!bin/build && target/handmade")
	vim.cmd("!bin/build")
end, {})
