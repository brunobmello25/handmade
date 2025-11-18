vim.keymap.set("n", "<M-m>", function()
	-- vim.cmd("!bin/build && target/handmade")
	vim.cmd("!BUILD_PLATFORM=1 USE_BEAR=1 bin/build")
end, {})

local build_augroup = vim.api.nvim_create_augroup("build", { clear = true })
vim.api.nvim_create_autocmd("BufWritePost", {
	group = build_augroup,
	callback = function()
		vim.system({ "bin/build" }, {
			cwd = vim.fn.getcwd(),
			text = false,
		})
	end,
})

local buildtest_augroup = vim.api.nvim_create_augroup("buildtest", { clear = true })
vim.api.nvim_create_autocmd("BufWritePost", {
	group = buildtest_augroup,
	callback = function()
		vim.system({ "bin/build", "test" }, {
			cwd = vim.fn.getcwd(),
			text = false,
		})
	end,
})

vim.api.nvim_create_user_command("BuildTest", function()
	vim.cmd("!bin/build test")
end, {})

local dap = require("dap")

dap.adapters.gdb = {
	type = "executable",
	command = "gdb",
	args = { "-i", "dap" },
}

dap.configurations.cpp = {
	{
		name = "Debug Game (codelldb)",
		type = "codelldb",
		request = "launch",
		program = vim.fn.getcwd() .. "/target/handmade",
		cwd = vim.fn.getcwd(),
		stopOnEntry = false,
	},
	{
		name = "Debug Tests (codelldb)",
		type = "codelldb",
		request = "launch",
		program = vim.fn.getcwd() .. "/target/handmade_test",
		cwd = vim.fn.getcwd(),
		stopOnEntry = false,
	},
	{
		name = "Debug Game (gdb)",
		type = "gdb",
		request = "launch",
		program = vim.fn.getcwd() .. "/target/handmade",
		cwd = vim.fn.getcwd(),
		stopOnEntry = false,
	},
	{
		name = "Debug Tests (gdb)",
		type = "gdb",
		request = "launch",
		program = vim.fn.getcwd() .. "/target/handmade_test",
		cwd = vim.fn.getcwd(),
		stopOnEntry = false,
	},
}

-- User command to run and debug tests
vim.api.nvim_create_user_command("DebugTest", function()
	local dap_ok_cmd, dap_cmd = pcall(require, "dap")
	if dap_ok_cmd then
		vim.cmd("!bin/build test")
		vim.defer_fn(function()
			-- Select the "Debug Tests" configuration
			for _, config in ipairs(dap_cmd.configurations.cpp or {}) do
				if config.name:match("Debug Tests") then
					dap_cmd.run(config)
					return
				end
			end
			dap_cmd.continue()
		end, 500)
	else
		print("nvim-dap not installed")
	end
end, {})
