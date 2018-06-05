#!/usr/bin/python
import os
import signal
import pwd
import re
import time
import commands

# Shutdown timer (seconds)
shutdown_interval = 600

# Current volume
current_volume = '0%'

# Get PID of pulseaudio
def get_pid_of_pulseaudio():
	pid_list = [pid for pid in os.listdir('/proc') if pid.isdigit()]
	username = ''

	for pid in pid_list:
		uid_path = '/proc/' + pid + '/loginuid'
		comm_path = '/proc/' + pid + '/comm'

		try:
			uid = ''
			with open(uid_path, 'r') as fp:
				uid = fp.read()

			comm = ''
			with open(comm_path, 'r') as fp:
				comm = fp.read()

			# Skip system or root processes`
			if (int(uid) < 1000) or (int(uid) == 4294967295):
				continue

			if comm == 'pulseaudio\n':
				return pid
		except:
			continue

	return username


# Get env of pulseaudio
def get_env_of_pulseaudio():
	pid = get_pid_of_pulseaudio()
	env = ''
	
	env_path = '/proc/' + pid + '/environ'
	try:
		with open(env_path, 'r') as fp:
			env = fp.read()
	except:
		env = ''
	
	return env


# Get username of pulseaudio
def get_username_of_pulseaudio():
	pid = get_pid_of_pulseaudio()
	username = ''
	
	uid_path = '/proc/' + pid + '/loginuid'
	comm_path = '/proc/' + pid + '/comm'

	try:
		uid = None
		with open(uid_path, 'r') as fp:
			uid = fp.read()

		comm = None
		with open(comm_path, 'r') as fp:
			comm = fp.read()

		if comm == 'pulseaudio\n':
			username = pwd.getpwuid(int(uid))
			username = username[0]
	except:
		username = ''

	return username


# Get Seat Env
def get_seat_env(env):
	match = ''
	p = re.compile('XDG_SEAT_PATH=.*Seat.')
	m = p.search(env)
	if m:
		 match = m.group()

	return match


# Get Session Env
def get_dbus_env(env):
	match = ''
	p = re.compile('DBUS_SESSION_BUS_ADDRESS=.*/bus')
	m = p.search(env)
	if m:
		match = m.group()

	p = re.compile('XDG_RUNTIME_DIR=.*/[0-9]+')
	m = p.search(env)
	if m:
		match = match + ' ' + m.group()

	p = re.compile('DISPLAY=:.*[0-9]{1}\.[0-9]{1}')
	m = p.search(env)
	if m:
		match = match + ' ' + m.group()

	p = re.compile('XDG_SEAT=seat.')
	m = p.search(env)
	if m:
		match = match + ' ' + m.group()

	p = re.compile('XDG_SESSION_ID=[0-9]+')
	m = p.search(env)
	if m:
		match = match + ' ' + m.group()

	return match


# Send signal to all user processes
def send_signal(signal):
        pid_list = [pid for pid in os.listdir('/proc') if pid.isdigit()]

	for pid in pid_list:
		try:
			uid_path = '/proc/' + pid + '/loginuid'
			comm_path = '/proc/' + pid + '/comm'

			uid = None
			with open(uid_path, 'r') as fp:
				uid = fp.read()

			comm = ''
			with open(comm_path, 'r') as fp:
				comm = fp.read()

			# Skip system or root processes`
			if (int(uid) < 1000) or (int(uid) == 4294967295):
				continue

			# Skip userlevel service processes`
			if ('xfce4' in comm or 'pulseaudio' in comm):
				continue

			os.kill(int(pid), signal)
		except:
			continue


# Mute sound
def mute_sound():
	global current_volume

	env = get_env_of_pulseaudio()
	dbus_env = get_dbus_env(env)
	username = get_username_of_pulseaudio()

	# Get current volume
	result =  commands.getstatusoutput('runuser -l %s -c "export %s; amixer get Master"' % 
		(username, dbus_env))

	match = ''
	p = re.compile('\[[0-9]+%\]')
	m = p.search(result[1])
	if m:
		match = m.group()
		current_volume = match.replace('[', '').replace(']','')
	else:
		current_volume = '50%'

	# Set volumen to 0%
	os.system('runuser -l %s -c "export %s; amixer -q set Master 0%%"; sleep 1' % 
		(username, dbus_env))


# Unmute sound
def unmute_sound():
	global current_volume

	env = get_env_of_pulseaudio()
	dbus_env = get_dbus_env(env)
	username = get_username_of_pulseaudio()

	# Restore volumn
	os.system('runuser -l %s -c "export %s; amixer -q set Master %s"' % 
		(username, dbus_env, current_volume))


# Lock screen
def lock_screen():
	env = get_env_of_pulseaudio()
	seat_env = get_seat_env(env)
	username = get_username_of_pulseaudio()

	# Lock screen
	os.system('runuser -l %s -c "export %s; dm-tool lock"' % (username, seat_env))


# Enter sleep
def enter_sleep():
	# Wait for lock screen
	os.system('sleep 3')

	send_signal(signal.SIGSTOP)


# Wake up
def wake_up():
	# Wait for lock screen
	os.system('sleep 2')

	send_signal(signal.SIGCONT)


# Main function
if __name__ == '__main__':
	closed = False

	while True:
		fp = open('/proc/acpi/button/lid/LID0/state', 'r')
		data = fp.read()

		# Enter sleep
		if 'closed' in data and closed == False:
			print "Enter Sleep..."
			mute_sound()
			enter_sleep()
			closed = True
			start_time = time.time()

		# Wake up
		elif 'open' in data and closed == True:
			print "Wake up..."
			wake_up()
			unmute_sound()
			closed = False

		# Shutdown if time is over
		elif closed == True:
			elipse = time.time() - start_time
			if (elipse > shutdown_interval):
				print "Shutdown..."
				wake_up()
				os.system('sleep 5')

				# kill after recovering volume
				unmute_sound()
				os.system('killall pulseaudio')
				os.system('shutdown -h now')

		os.system('sleep 1')
		fp.close()
