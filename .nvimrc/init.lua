vim.keymap.set("n", "<M-m>", function()
	-- vim.cmd("!bin/build && target/handmade")
	vim.cmd("!BUILD_PLATFORM=1 bin/build")
end, {})

local build_augroup = vim.api.nvim_create_augroup("build", { clear = true })
vim.api.nvim_create_autocmd("BufWritePost", {
	group = build_augroup,
	callback = function()
		vim.cmd("silent !bin/build > /dev/null 2>&1")
	end,
})
