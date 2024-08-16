# setup.py
from setuptools import setup, find_packages

setup(
    name='periplus-client',
    version='0.1',
    packages=find_packages(),
    install_requires=[
        # List your dependencies here
        "asyncio"
    ],
    entry_points={
        'console_scripts': [
            # Define any scripts you want to be executable
        ],
    },
    author='Quintin Leary',
    author_email='quintindleary@gmail.com',
    description='A client library for connecting with Periplus.',
    long_description=open('README.md').read(),
    long_description_content_type='text/markdown',
    url='https://github.com/QDL123/Periplus',
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.8',
)
