default['fds_cloud']['fds-admin_username'] = 'fds-admin'
default['fds_cloud']['fds-admin_groupname'] = 'fds-admin'
default['fds_cloud']['fds-admin_homedir'] = '/home/fds-admin'
default['fds_cloud']['fds-admin_authkey'] = 'ssh-dss AAAAB3NzaC1kc3MAAACBAKC0bvd7GjtksXWR2l0ZMNyzcXH9keg33NXXstUAq9y1cg0a88l7e/XOFxJVMSrvYB1kx2yIl59wTELE+/ROXq8Wdywu5O3pS5nyAWeRpjN0RgyfDtZ06uA0ahlUfdzLypsh+bmxmeCMR1So7R/11E40gYYoTgZiFus9zLnkyd3BAAAAFQDEXIlEgom7PAE7aujqzEGc1eDUtQAAAIB10jEHDOAcbZMYMKHXwODJm7NEHtXB4O0Uow7diTqSkI3QV8EKxIYc5ic5HcLQonEywLHp+3pmLpbrc0HEi1V342+eyureQLh7iUSvBKPw3961rZ7Inb26GcJZq/waDaK7hBsc07zM5IgxgekiI+qh/U330Pl9idaIAMNenygOnAAAAIEAg+kR/rZ8fveG01jou7TxZ8ysXP1xyXcsmLEs8Nxp1Mk1ZL9NPHIJTBfZIHpLwJWDOFqfElz25RXOjam1Y1Qe+FHuEHpoDQpoLh1tw+Axr7WPeQllt57qkXYbOCI/NOsUspJGYWpuadZAC1D0Zi+NkixQtrCH5Nkb1Ye2sDPpQuc= cboggs@ec2-mon-01'
default['sysctl']['params']['kernel']['core_pattern'] = "%e-%p.core"
default['sysctl']['params']['net']['ipv4']['tcp_tw_reuse'] = 1
default['sysctl']['params']['net']['ipv4']['tcp_tw_recycle'] = 1