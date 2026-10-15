#!/usr/bin/env python3
"""Create a private, per-owner SMS bootstrap. Does not contact the CPE."""
import argparse,getpass,ipaddress,os,secrets,subprocess
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('--device-ip',default='192.168.8.1')
p.add_argument('--client-network',default='192.168.8.0/24')
p.add_argument('--output',type=Path,required=True,help='private directory outside the public repository')
a=p.parse_args();address=ipaddress.ip_address(a.device_ip);network=ipaddress.ip_network(a.client_network)
assert address.version==4 and address.is_private and address in network
password=getpass.getpass('Set your temporary/persistent stock SSH password: ')
assert len(password)>=12 and password==getpass.getpass('Repeat password: '),'Passwords must match and contain at least12 characters'
args=['openssl','passwd','-1','-salt',secrets.token_hex(4),'-stdin']
if os.name=='nt':args=['wsl','-d','Debian','--']+args
hashed=subprocess.check_output(args,input=(password+'\n').encode()).decode().strip()
assert hashed.startswith('$1$') and '\n' not in hashed
template=Path(__file__).with_name('stock-ssh-bootstrap.sh.in').read_text(encoding='utf-8')
text=template.replace('@DEVICE_IP@',str(address)).replace('@CLIENT_NETWORK@',str(network)).replace('@PASSWORD_HASH@',hashed)
a.output.mkdir(parents=True,exist_ok=True)
target=a.output/'lg6851f-stock-ssh-bootstrap.sh';target.write_text(text,encoding='utf-8',newline='\n');target.chmod(0o600)
print('Private payload created:',target)
print('Do not upload this generated file: it contains your password hash.')

