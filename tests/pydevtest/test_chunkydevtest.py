import subprocess
        # test imeta -v
        imeta_output = subprocess.check_output('echo "ls -d icmdtest/foo1" | imeta -v', shell=True)
        assert imeta_output.find('testmeta1') > -1
        assert imeta_output.find('180') > -1
        assert imeta_output.find('cm') > -1
        