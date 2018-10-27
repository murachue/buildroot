#!/usr/bin/env ruby
# coding: utf-8

# Proxy for N64Linux + EverDrive-64
# usage: ruby $0 /dev/ttyUSB0 [[address:]port]

require 'io/console'
require 'socket'

def read_whole_timeout io, size, timeout
	buf = ''.b
	while buf.size < size
		rs, _, _ = IO.select([io], nil, nil, timeout)
		rs.nil? or rs.size == 0 and return buf
		buf += io.readpartial size - buf.size
	end
	return buf
end

def interact remote, local, log, pipe
	loop {
		rs, _, es = IO.select([remote, local[:r]], nil, [remote, local[:r]])
		if not es.empty?
			$stderr.puts es.inspect
			break
		end
		if rs.include?(remote)
			r = read_whole_timeout(remote, 512, 0.5)
			log and $stderr.puts "> #{r.inspect}"
			r = r[1, r.unpack("C").first]
			!pipe and r.gsub!(/([^\r])\n/, "\\1\r\n") # convert LF -> CRLF
			local[:w].write(r)
		end
		if rs.include?(local[:r])
			local[:r].eof? and break
			r = local[:r].readpartial(255)
			if !pipe and r == "!!b\n"
				# send Break
				r = ""
			end
			if !pipe and r == ""
				# quit (required on RAW tty mode)
				break
			end
			r = [r.size, r].pack("Ca511")
			log and $stderr.puts "< #{r.inspect}"
			remote.write(r)
		end
	}
end

File.open(ARGV[0], 'r+b') { |com|
	com.raw!
	com.sync = true

	if ARGV[1] and ARGV[1] != "0" and ARGV[1] =~ /\A(?:(.+):)?(.+)\z/
		addr = $1 || '127.0.0.1'
		port = $2
		TCPServer.open(addr, port) { |ss|
			loop {
				puts "listening #{addr}:#{port}"
				s = ss.accept
				puts "accept from #{s.peeraddr.values_at(3,1).inspect}"
				interact com, {r: s, w: s}, true, true
				puts "disconnected"
			}
		}
	else
		$stdout.puts "com64 ready"
		$stdout.sync = true
		$stdin.raw!
		begin
			$stdin.sync = true
			interact com, {r: $stdin, w: $stdout}, false, !!ARGV[1]
		ensure
			$stdout.puts "ESCAPE!"
			$stdin.cooked!
		end
	end
}
