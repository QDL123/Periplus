# setup.py
from setuptools import setup, find_packages

setup(
    name='periplus_proxy',
    version='0.1',
    packages=find_packages(),
    install_requires=[
        # List your dependencies here
        "pydantic",
        "fastapi",
        "uvicorn"
    ],
    entry_points={
        'console_scripts': [
            # Define any scripts you want to be executable
        ],
    },
    author='Quintin Leary',
    author_email='quintindleary@gmail.com',
    description='A proxy for Periplus to load data through.',
    long_description=open('README.md').read(),
    long_description_content_type='text/markdown',
    url='https://github.com/yourusername/mypackage',
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.6',
)
