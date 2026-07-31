from setuptools import find_packages, setup

package_name = "ah_yolo"

setup(
    name=package_name,
    version="0.1.0",
    packages=find_packages(exclude=["test"]),
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/" + package_name]),
        ("share/" + package_name, ["package.xml"]),
        ("share/" + package_name + "/config", ["config/ah_yolo_params.yaml"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="AeroHub",
    maintainer_email="dev@aerohub.local",
    description="AeroHub YOLO smart-detection node (Ultralytics)",
    license="Apache-2.0",
    entry_points={
        "console_scripts": [
            "ah_yolo_node = ah_yolo.ah_yolo_node:main",
        ],
    },
)
