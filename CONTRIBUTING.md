<!-- omit in toc -->
# Contributing to Periplus

First off, thanks for taking the time to contribute! ❤️

All types of contributions are encouraged and valued. See the [Table of Contents](#table-of-contents) for different ways to help and details about how this project handles them. Please make sure to read the relevant section before making your contribution. It will make it a lot easier for us maintainers and smooth out the experience for all involved. The community looks forward to your contributions. 🎉

> And if you like the project, but just don't have time to contribute, that's fine. There are other easy ways to support the project and show your appreciation, which we would also be very happy about:
> - Star the project
> - Tweet about it
> - Refer this project in your project's readme
> - Mention the project at local meetups and tell your friends/colleagues

<!-- omit in toc -->
## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [I Have a Question](#i-have-a-question)
  - [I Want To Contribute](#i-want-to-contribute)
  - [Reporting Bugs](#reporting-bugs)
  - [Suggesting Enhancements](#suggesting-enhancements)
  - [Your First Code Contribution](#your-first-code-contribution)
- [Styleguides](#styleguides)
  - [Commit Messages](#commit-messages)
- [Join The Project Team](#join-the-project-team)


## Code of Conduct

This project and everyone participating in it is governed by the
[Periplus Code of Conduct](/blob/main/CODE_OF_CONDUCT.md).
By participating, you are expected to uphold this code. Please report unacceptable behavior
to <periplus.maintainers@gmail.com>.


## I Have a Question

> If you want to ask a question, we assume that you have read the available [Documentation](/clients/python/docs/api_reference.md).

Before you ask a question, it is best to search for existing [Issues](/issues) that might help you. In case you have found a suitable issue and still need clarification, you can write your question in this issue. It is also advisable to search the internet for answers first.

If you then still feel the need to ask a question and need clarification, we recommend the following:

- Open an [Issue](/issues/new).
- Provide as much context as you can about what you're running into.
- Provide project and platform versions, depending on what seems relevant.

We will then take care of the issue as soon as possible.

<!--
You might want to create a separate issue tag for questions and include it in this description. People should then tag their issues accordingly.

Depending on how large the project is, you may want to outsource the questioning, e.g. to Stack Overflow or Gitter. You may add additional contact and information possibilities:
- IRC
- Slack
- Gitter
- Stack Overflow tag
- Blog
- FAQ
- Roadmap
- E-Mail List
- Forum
-->

## I Want To Contribute

> ### Legal Notice <!-- omit in toc -->
> When contributing to this project, you must agree that you have authored 100% of the content, that you have the necessary rights to the content and that the content you contribute may be provided under the project licence.

### Reporting Bugs

<!-- omit in toc -->
#### Before Submitting a Bug Report

A good bug report shouldn't leave others needing to chase you up for more information. Therefore, we ask you to investigate carefully, collect information and describe the issue in detail in your report. Please complete the following steps in advance to help us fix any potential bug as fast as possible.

- Make sure that you are using the latest version.
- Determine if your bug is really a bug and not an error on your side e.g. using incompatible environment components/versions (Make sure that you have read the [documentation](clients/python/docs/api_reference.md). If you are looking for support, you might want to check [this section](#i-have-a-question)).
- To see if other users have experienced (and potentially already solved) the same issue you are having, check if there is not already a bug report existing for your bug or error in the [bug tracker](/issues?q=label%3Abug).
- Also make sure to search the internet (including Stack Overflow) to see if users outside of the GitHub community have discussed the issue.
- Collect information about the bug:
  - Stack trace (Traceback)
  - OS, Platform and Version (Windows, Linux, macOS, x86, ARM)
  - Version of the interpreter, compiler, SDK, runtime environment, package manager, depending on what seems relevant.
  - Possibly your input and the output
  - Can you reliably reproduce the issue? And can you also reproduce it with older versions?

<!-- omit in toc -->
#### How Do I Submit a Good Bug Report?

> You must never report security related issues, vulnerabilities or bugs including sensitive information to the issue tracker, or elsewhere in public. Instead sensitive bugs must be sent by email to <periplus@gmail.com>. <!-- Replace with your project's security contact email. -->
<!-- You may add a PGP key to allow the messages to be sent encrypted as well. -->

We use GitHub issues to track bugs and errors. If you run into an issue with the project:

- Open an [Issue](/issues/new). (Since we can't be sure at this point whether it is a bug or not, we ask you not to talk about a bug yet and not to label the issue.)
- Explain the behavior you would expect and the actual behavior.
- Please provide as much context as possible and describe the *reproduction steps* that someone else can follow to recreate the issue on their own. This usually includes your code. For good bug reports you should isolate the problem and create a reduced test case.
- Provide the information you collected in the previous section.

Once it's filed:

- The project team will label the issue accordingly.
- A team member will try to reproduce the issue with your provided steps. If there are no reproduction steps or no obvious way to reproduce the issue, the team will ask you for those steps and mark the issue as `needs-repro`. Bugs with the `needs-repro` tag will not be addressed until they are reproduced.
- If the team is able to reproduce the issue, it will be marked `needs-fix`, as well as possibly other tags (such as `critical`), and the issue will be left to be [implemented by someone](#your-first-code-contribution).

<!-- You might want to create an issue template for bugs and errors that can be used as a guide and that defines the structure of the information to be included. If you do so, reference it here in the description. -->


### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for Periplus, **including completely new features and minor improvements to existing functionality**. Following these guidelines will help maintainers and the community to understand your suggestion and find related suggestions.

<!-- omit in toc -->
#### Before Submitting an Enhancement

- Make sure that you are using the latest version.
- Read the [documentation](/blob/main/README.md) carefully and find out if the functionality is already covered, maybe by an individual configuration.
- Perform a [search](/issues) to see if the enhancement has already been suggested. If it has, add a comment to the existing issue instead of opening a new one.
- Find out whether your idea fits with the scope and aims of the project. It's up to you to make a strong case to convince the project's developers of the merits of this feature. Keep in mind that we want features that will be useful to the majority of our users and not just a small subset. If you're just targeting a minority of users, consider writing an add-on/plugin library.

<!-- omit in toc -->
#### How Do I Submit a Good Enhancement Suggestion?

Enhancement suggestions are tracked as [GitHub issues](/issues).

- Use a **clear and descriptive title** for the issue to identify the suggestion.
- Provide a **step-by-step description of the suggested enhancement** in as many details as possible.
- **Describe the current behavior** and **explain which behavior you expected to see instead** and why. At this point you can also tell which alternatives do not work for you.
- You may want to **include screenshots or screen recordings** which help you demonstrate the steps or point out the part which the suggestion is related to. You can use [LICEcap](https://www.cockos.com/licecap/) to record GIFs on macOS and Windows, and the built-in [screen recorder in GNOME](https://help.gnome.org/users/gnome-help/stable/screen-shot-record.html.en) or [SimpleScreenRecorder](https://github.com/MaartenBaert/ssr) on Linux. <!-- this should only be included if the project has a GUI -->
- **Explain why this enhancement would be useful** to most Periplus users. You may also want to point out the other projects that solved it better and which could serve as inspiration.

<!-- You might want to create an issue template for enhancement suggestions that can be used as a guide and that defines the structure of the information to be included. If you do so, reference it here in the description. -->

### Your First Code Contribution

- **Setup your development environment**:
  - Fork the Repository: Go to the Periplus repository on GitHub and click the "Fork" button at the top-right of the page. This will create a copy of the repository under your own GitHub account.
  - Clone your fork: `git clone https://github.com/your-username/Periplus.git`
  - Navigate to the repository: `cd Periplus`
  - Set up the development environment based on the project requirements:
    - For the Periplus Server:
      - Install dependencies using Homebrew: `brew install faiss curl cpr rapidjson libomp catch2 cmake`
      - Build the project using CMake:
        ```bash
        cmake -S . -B build
        cmake --build build
        ```
    - For the Periplus Client Library:
      - Ensure Python 3.8+ is installed.
      - Create and activate your virtual environment:
        ```bash
        python -m venv env
        source venv/bin/activate
        ```
      - You can install the local version of the client library in editable mode by running:
      `pip install -e <path-to-project-root>/clients/python`
    - For the Periplus Proxy:
      - Ensure Python 3.8+ is installed.
      - Create and activate your virtual environment:
        ```bash
        python -m venv env
        source venv/bin/activate
        ```
      - You can install the local version of the proxy in editable mode by running:
      `pip install -e <path-to-project-root>/clients/python`
  - Make your changes in a new branch: `git checkout -b feature/your-feature-name`
  - Run tests:
    - For the server, run the test executable with `./build/tests`.
    - Run the e2e tests:
      - Set up the test proxy/db
        - Navigate to the e2e test directory: `cd test/e2e`
        - Create a virtual environment:
          ```bash
          python -m venv test_proxy_env
          source venv/bin/activate
          pip install -r proxy_requirements.txt # This will install the local version of the proxy package
          ```
        - Start up the test proxy (For test purposes the db is built into the proxy): `python test_proxy.py`
      - Run the Periplus server
        - In another shell, navigate back to the root of the project `cd /<path-to-repo>/Periplus`
        - Compile Periplus if you haven't already: `cmake --build build`
        - Run the executable (the test expects port 3000): `./build/periplus -p 3000`
      - Run the test driver
        - The test driver requires a conda environment. If you don't already have anaconda installed, miniconda is recommended as it doesn't come with a bunch of unnecessary preinstalled packages. You can find the installation instructions [here](https://docs.anaconda.com/miniconda/miniconda-install/).
        - In a third shell, navigate back to the test directory: `cd test/e2e`
        - Create and activate the virtual environment:
          ```bash
          conda env create -f environment.yml
          conda activate e2e_env
          ```
        - Run the test driver: `python e2e_test.py` 


  - Commit your changes: `git commit -m 'Add feature X'`
  - Push to your branch: `git push origin feature/your-feature-name`
  - Create a Pull Request with a detailed description of your changes.

## Styleguides
### Commit Messages

- **Commit Message Guidelines**:
  - Use the present tense ("Add feature" not "Added feature").
  - Keep the commit message concise but descriptive.
  - Include a detailed body when necessary, explaining the "why" behind the change.
  - Use tags to indicate the type of change: 
    - `fix`: Bug fix.
    - `feat`: New feature.
    - `docs`: Documentation changes.
    - `refactor`: Code refactoring.
    - `test`: Adding or updating tests.
  - Example:
    ```
    feat: Add support for new caching strategy

    Introduced a new least-recently-used (LRU) caching strategy to improve performance.
    ```

## Join The Project Team

- **Becoming a Maintainer**:
  - We’re always looking for motivated contributors to join the Periplus project team!
  - To become a maintainer, you should:
    - Have a history of valuable contributions to the project.
    - Show consistent involvement and understanding of the codebase.
    - Demonstrate leadership in helping others, reviewing PRs, and guiding discussions.
  - If you’re interested in joining the team, reach out to the current maintainers by opening an issue or sending an email to <periplus.maintainers@gmail.com> with details about your contributions and why you want to join.

<!-- omit in toc -->
## Attribution
This guide is based on the **contributing-gen**. [Make your own](https://github.com/bttger/contributing-gen)!

---