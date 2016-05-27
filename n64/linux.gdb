define lpath
	set $de = $arg0->f_path.dentry
	while $de.d_name.name[0] != '/'
		printf "%s,", $de.d_name.name
		set $de = $de.d_parent
	end
end
define lshowm
	printf "%08x-%08x", $m->vm_start, $m->vm_end
	if $m->vm_file
		printf " "
		lpath $m->vm_file
		printf "+%05x(%x)", $m->vm_pgoff, $m->vm_file->f_pos
	end
end
define lmap
	set $m = runqueues->curr->mm->mmap
	while $m
		lshowm
		printf "\n"
		set $m = $m->vm_next
	end
end
define lin
	printf "%08x @ ", $arg0
	set $m = runqueues->curr->mm->mmap
	while $m
		if $m->vm_start <= $arg0 && $arg0 < $m->vm_end
			lshowm
			if $m->vm_file
				printf " file offset %08x\n", $arg0 - $m->vm_start + ($m->vm_pgoff << 12)
			end
			loop_break
		else
			if $arg0 < $m->vm_end
				set $m = 0
			else
				set $m = $m->vm_next
			end
		end
	end
	if !$m
		printf "?\n"
	end
end
define lsave
	set $l_ppc = $pc
	set $i = 0
	while $i < 32
		printf "."
		eval "set $l_p%d = $r%d", $i, $i
		set $i = $i + 1
	end
	printf "\n"
end
define lload
	set $pc = $l_ppc
	set $i = 0
	while $i < 32
		printf "."
		eval "set $r%d = $l_p%d", $i, $i
		set $i = $i + 1
	end
	printf "\n"
end
define luser
	# first, save it to temporary conv-vars to avoid losing "regs" symbol
	set $l_tpc = regs->cp0_epc
	set $i = 0
	while $i < 32
		printf "."
		eval "set $l_t%d = %d", $i, regs->regs[$i]
		set $i = $i + 1
	end
	printf "\n"

	# then, set it.
	set $i = 0
	while $i < 32
		printf "."
		eval "set $r%d = $l_t%d", $i, $i
		set $i = $i + 1
	end
	printf "."
	set $pc = $l_tpc
	printf "\n"
end
define lps
	set $l_task = &init_task
	while 1
		printf "%5d %s\n", $l_task->pid, $l_task->comm
		set $l_task = (struct task_struct*)((int)$l_task->tasks->next - (int)&(((struct task_struct*)0)->tasks->next))
		if $l_task == &init_task
			loop_break
		end
	end
end
define lcon
	set architecture mips:4300
	target remote 192.168.56.1:23946
end
define lstep
	set $sr|=1<<23
	c
end
define lboot
	set architecture mips:4300
	file
	tb *0xa40004b8
	c
	tb *0x800002ac
	c
	si
end
define lsecond
	tb *0x80000470
	c
	tb *0x803ff0c4
	c
	si
end
define lentry
	lboot
	lsecond
	file build/linux-custom/vmlinux
end
