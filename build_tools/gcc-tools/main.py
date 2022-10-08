import argparse
import subprocess
import os
import logging
import getpass
from paramiko import SSHClient, AutoAddPolicy, RSAKey, SSHException

logging.basicConfig(level = logging.INFO, format = '%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger()

# set global param for other function to use
USER = ""
PASSWD = ""
SKEY = ""
IP = ""
BASEDST = ""
WHO = ""
XTOOLSDIR = ""
CROSSDIR = ""
SSHCLI = SSHClient()
SFTPCLI = None
ARCHLIST = []


def init_args():
    """
    init args
    :return: parse_args
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("-u", type=str, dest="user", default="", required=False)
    parser.add_argument("-p", type=str, dest="passwd", default="", required=False)
    parser.add_argument("-skey", type=str, dest="skey", default="", required=False)
    parser.add_argument("-ip", type=str, dest="ip", default="", required=False)
    parser.add_argument("-dst", type=str, dest="dst", default="", required=False)
    parser.add_argument("-archs", type=str, dest="archs", default="", required=False)
    parser.add_argument("-test", type=bool, dest="is_test", default=False, required=False)
    
    return parser.parse_args()


def release_ssh():
    '''
    release ssh resource
    '''
    global SSHCLI, SFTPCLI

    SSHCLI.close()
    SFTPCLI.close()


def main():
    # init param
    args = init_args()
    
    global USER, PASSWD, IP, SKEY, BASEDST, WHO, XTOOLSDIR, CROSSDIR
    USER = args.user
    PASSWD = args.passwd
    SKEY = args.skey
    IP = args.ip
    BASEDST = args.dst
    WHO = getpass.getuser()
    XTOOLSDIR = os.path.join("/home", WHO, "x-tools")
    CROSSDIR = os.path.join(os.getcwd(), "cross_tools")
    get_arch_list(args.archs)
    
    # check ssh param if currented
    if not check_param():
        logger.error("param check is not pass")
        return
    
    if args.is_test:
        for_test()
        release_ssh()
        return
    
    # running gcc compile brfore initing environment with this function
    if not prepare(cwd = CROSSDIR):
        return
    
    # running all gcc compile tasks
    if not compile_gcc_all():
        return

    command = "ls -al"    
    output = subprocess.run(
        command, 
        shell = True, 
        encoding = "utf-8", 
        cwd = XTOOLSDIR)
    if output.returncode == 0:
        logger.info(output.stdout)
    
    # tar gcc to tar.gz
    if not tar_and_upload_all():
        return
    
    logger.info("all task finishd successful")
    
    release_ssh()


def check_param():
    global USER, PASSWD, SKEY, IP, ARCHLIST
    if USER == "":
        logger.error("-u can not empty")
        return False
    
    if PASSWD == "" and SKEY == "":
        logger.error("-p and -skey can not empty together")
        return False
    
    if IP == "":
        logger.error("-ip can not empty")
        return False

    if len(ARCHLIST) == 0:
        logger.error("-archs can not empty, you must select from aarch64, arm32, x86_64 or riscv64 one or more")
        return False
    
    if not check_ssh():
        return False
    
    return True
    
    
def get_arch_list(archs : str):
    global ARCHLIST
    split = archs.split(" ")
    for arch in split:
        ARCHLIST.append(arch)


def for_test():
    global SFTPCLI, ARCHLIST
    listdir = SFTPCLI.listdir()
    logger.info(listdir)
    
    logger.info(ARCHLIST)


def check_ssh():
    '''
    check ssh param if current
    '''
    global USER, PASSWD, SKEY, IP, SSHCLI, SFTPCLI
    
    SSHCLI.set_missing_host_key_policy(AutoAddPolicy)
    
    try:
        if SKEY == "":    
            SSHCLI.connect(hostname = IP, username = USER, password = PASSWD)
        else:
            pri_key = RSAKey.from_private_key_file(SKEY)
            SSHCLI.connect(hostname = IP, username = USER, pkey=pri_key)
            
        SFTPCLI = SSHCLI.open_sftp()
    except SSHException:
        logger.error("ssh init faild")
    
    return True


def prepare(cwd):
    '''
    running .prepare.sh
    '''
    logger.info("====================now running prepare==========================================")
    command = "./prepare.sh"
    with subprocess.Popen(
        command, 
        shell = True, 
        encoding = "utf-8", 
        stdout = subprocess.PIPE, 
        cwd = cwd) as proc:

        stdout = proc.stdout      
        for readline in stdout:
            if readline.find("'cp config_riscv64 .config && ct-ng build' for build riscv64") != -1:
                break
            logger.info(readline)
        stdout.close()

    logger.info("====================prepare successful==========================================")
    
    return True


def compile_gcc_all():
    global ARCHLIST, CROSSDIR
    
    if "aarch64" in ARCHLIST:
        if not compile_gcc("aarch64", CROSSDIR):
            return False
    
    if "arm32" in ARCHLIST:
        if not compile_gcc("arm32", CROSSDIR):
            return False
    
    if "x86_64" in ARCHLIST:
        if not compile_gcc("x86_64", CROSSDIR):
            return False
        
    if "riscv64" in ARCHLIST:
        if not compile_gcc("riscv64", CROSSDIR):
            return False
    
    return True


def compile_gcc(arch, cwd):
    logger.info("====================now building gcc-{}====================================".format(arch))
    command = "cp config_{} .config && ct-ng build".format(arch)
    with subprocess.Popen(
        command, 
        shell = True, 
        encoding = "utf-8", 
        stdout = subprocess.PIPE, 
        cwd = cwd) as proc:
        
        stdout = proc.stdout   
        for readline in stdout:
            logger.info(readline)
            if readline.find("Finishing installation") != -1:
                break
        stdout.close()

    logger.info("====================build gcc-{} successful====================================".format(arch))
    
    return True


def tar_and_upload_all():

    global ARCHLIST, XTOOLSDIR
    
    if "aarch64" in ARCHLIST:
        if not tar_and_upload("aarch64-openeuler-linux-gnu", "openeuler_gcc_arm64le", XTOOLSDIR):
            return False

    if "arm32" in ARCHLIST:
        if not tar_and_upload("arm-openeuler-linux-gnueabi", "openeuler_gcc_arm32le", XTOOLSDIR):
            return False
    
    if "x86_64" in ARCHLIST:
        if not tar_and_upload("x86_64-openeuler-linux-gnu", "openeuler_gcc_x86_64", XTOOLSDIR):
            return False

    if "riscv64" in ARCHLIST:    
        if not tar_and_upload("riscv64-openeuler-linux-gnu", "openeuler_gcc_riscv64", XTOOLSDIR):
            return False
    
    return True


def tar_and_upload(origin, target, cwd):
    # mv origin directory to target directory as we want for tar step
    command = "mv {} {} && tar zcf {}.tar.gz {}".format(origin, target, target, target)
    output = subprocess.run(
        command, 
        shell = True, 
        encoding = "utf-8", 
        cwd = cwd)
    if output.returncode != 0:
        logger.info("{} tar step faield".format(target))
        logger.error(output.stderr)
        return False
    logger.info("finished {} tar step".format(target))
    
    # upload action
    global BASEDST
    source = os.path.join(cwd, target + ".tar.gz")
    dst = os.path.join(BASEDST, "gcc")
    if not upload(source = source, dst = dst):
        logger.error("upload {}.tar.gz faield".format(target))
        return False 
    
    logger.info("upload {}.tar.gz successful".format(target))
    return True


def upload(source, dst):
    '''
    upload to remote server
    '''
    global SSHCLI, SFTPCLI, BASEDST
    
    filename = os.path.basename(source)
    dst_file = os.path.join(dst, filename)

    try:
        listfile = SFTPCLI.listdir(dst_file)
        if len(listfile) > 0:
            # delete dst directory
            SSHCLI.exec_command("rm -rf {}".format(listfile))
        SSHCLI.exec_command("mkdir -p {}".format(dst))
        logger.info("mkdir {} action successful".format(dst))
    except FileNotFoundError:
        SSHCLI.exec_command("mkdir -p {}".format(dst))
        logger.info("mkdir {} action successful".format(dst))
    
    # upload local resource to remote server
    SFTPCLI.put(source, dst_file)
    logger.info("put {} action successful".format(dst))
    
    return True


if __name__ == "__main__":
    main()