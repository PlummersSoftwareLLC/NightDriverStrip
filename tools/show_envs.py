import configparser

config = configparser.ConfigParser()
config.read('platformio.ini')

envs = ''


for env in config.sections():
    if env.startswith('env:'):
        if envs != '':
            envs += ','
        envs += '"' + env[4::] + '"'

print('[' + envs + ']')