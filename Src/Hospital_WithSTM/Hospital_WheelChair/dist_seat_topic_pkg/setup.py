from setuptools import find_packages, setup

package_name = 'dist_seat_topic_pkg'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ubuntu',
    maintainer_email='pi@todo.todo',
    description='TODO: Package description',
    license='TODO: License declaration',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [   
            #'hostname_republisher = dist_seat_topic_pkg.hostname_republisher:main',
            'stm32_bridge_all = dist_seat_topic_pkg.stm32_bridge_all:main',
        ],
    },
)
