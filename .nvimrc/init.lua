vim.keymap.set("n", "<leader>hm", function()
	vim.cmd("!bin/build && target/handmade")
end, {})
