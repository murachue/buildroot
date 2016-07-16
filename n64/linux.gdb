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
#set $l_nexttask = (struct task_struct*)((int)$arg0->tasks->next - (int)&(((struct task_struct*)0)->tasks->next))
define lps_slow
	printf "PID__ PPID_ R mm______ th COMM___________ task____ stack___ sp______ ra______\n"
	set $l_task = &init_task
	while 1
		set $l_thr = $l_task->thread_group->next
		set $l_nthrs = 1
		while $l_thr != &$l_task->thread_group
			set $l_thr = $l_thr->next
			set $l_nthrs = $l_nthrs + 1
		end
		#set $l_pc = 0
		##should not happen...
		#if ((struct thread_info*)$l_task->stack)
		#	#should happen, never switched process.
		#	if ((struct thread_info*)$l_task->stack)->regs
		#		set $l_pc = ((struct thread_info*)$l_task->stack)->regs->regs[31]
		#	end
		#end
		printf "%5d %5d %c %08x %2d %-15s %08x %08x %08x %08x\n", $l_task->pid, $l_task->parent->pid, ($l_task == runqueues->curr ? 'R' : $l_task->on_rq ? 'r' : 's'), $l_task->mm, $l_nthrs, $l_task->comm, $l_task, $l_task->stack, $l_task->thread->reg29, $l_task->thread->reg31
		set $l_task = (struct task_struct*)((int)$l_task->tasks->next - (int)&(((struct task_struct*)0)->tasks))
		if $l_task == &init_task
			loop_break
		end
	end
end
define lps
	printf "PID__ PPID_ R mm______ th COMM___________ task____ stack___ sp______ ra______ ra2_____ ra3_____\n"
	set $l_ptask = &init_task
	while 1
		set $l_task = *$l_ptask
		set $l_thr = $l_task.thread_group.next
		set $l_nthrs = 1
		while $l_thr != &$l_ptask->thread_group
			set $l_thr = $l_thr->next
			set $l_nthrs = $l_nthrs + 1
		end
		#set $l_pc = 0
		##should not happen...
		#if ((struct thread_info*)$l_task->stack)
		#	#should happen, never switched process.
		#	if ((struct thread_info*)$l_task->stack)->regs
		#		set $l_pc = ((struct thread_info*)$l_task->stack)->regs->regs[31]
		#	end
		#end
		printf "%5d %5d %c %08x %2d %-15s %08x %08x %08x %08x %08x %08x\n", $l_task.pid, $l_task.parent->pid, ($l_ptask == runqueues->curr ? 'R' : $l_task.on_rq ? 'r' : 's'), $l_task.mm, $l_nthrs, &$l_ptask->comm, $l_ptask, $l_task.stack, $l_task.thread.reg29, $l_task.thread.reg31, *(int*)($l_task.thread.reg29+52), *(int*)($l_task.thread.reg29+56+20)
		set $l_ptask = (struct task_struct*)((int)$l_task.tasks.next - (int)&(((struct task_struct*)0)->tasks))
		if $l_ptask == &init_task
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
define ldisco
	disco
	d
	file
end
define lkill
	k
	d
	file
end
define _lprintcc
	set $_ = $arg0
	if $_ == 0
		printf "?reservedcc0?"
	else
	if $_ == 1
		printf "?reservedcc1?"
	else
	if $_ == 2
		printf "uncached"
	else
	if $_ == 3
		# cachable noncoherent
		printf "noncoherent"
	else
	if $_ == 4
		# cacheable coherent exclusive
		printf "exclusive"
	else
	if $_ == 5
		# cacheable coherent exclusive on write
		printf "sharable"
	else
	if $_ == 6
		# cacheable coherent update on write
		printf "update"
	else
	if $_ == 7
		printf "?reservedcc7?"
	else
		printf "?cc?(%d)", $_
	end
	end
	end
	end
	end
	end
	end
	end
end
define _lprintbit
	set $_ = $arg0
	if $_
		printf "%c", $arg1
	else
		printf "%c", $arg2
	end
end
define lpte
	printf "%08x -> PFN %06x ", $arg0, $arg0 >> 11
	_lprintcc ($arg0>>8)&7
	printf " "
	_lprintbit $arg0&0x80 'D' 'd'
	_lprintbit $arg0&0x40 'V' 'v'
	_lprintbit $arg0&0x20 'G' 'g'
	printf " "
	_lprintbit $arg0&0x10 'M' 'm'
	_lprintbit $arg0&0x08 'A' 'a'
	_lprintbit $arg0&0x04 'W' 'w'
	_lprintbit $arg0&0x02 'R' 'r'
	_lprintbit $arg0&0x01 'P' 'p'
end
define lvm
	# vaddr = gggg_gggg ggpp_pppp pppe_0000 0000_0000
	# arch/mips/include/asm/pgtable-bits.h (R4300)
	# pte = ffff_ffff ffff_ffff ffff_fccc dvgm_awrp
	#       software: p=present r=read w=write a=accessed m=modified
	#       hardware: g=global v=valid d=dirty c=cache-coherency
	#                 f=page-frame-number
	# note: caching to $p makes speed up significantly.
	printf "vaddr=%08x ", $arg0
	set $p = (int(**)[2])pgd_current[0]
	printf "pgd_current[CPU0]=%08x ", $p
	set $p = $p[$arg0 >> 22]
	printf "pgd_entry[v31:22]=%08x\n", $p
	set $p = $p[($arg0 & 0x3FE000) >> 13]
	#printf "pte0/1 for vaddr: %08x,%08x\n", $p[0], $p[1]
	if !((int)$arg0 & 0x1000)
		printf "pte0 %08x: ", $arg0 & 0xFFFFe000
		set $v = $p[0]
		lpte $v
		printf "\n"
	else
		printf "pte1 %08x: ", ($arg0 & 0xFFFFe000) + 0x1000
		set $v = $p[1]
		lpte $v
		printf "\n"
	end
end
