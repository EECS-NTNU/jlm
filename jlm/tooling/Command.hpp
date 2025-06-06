/*
 * Copyright 2018 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_TOOLING_COMMAND_HPP
#define JLM_TOOLING_COMMAND_HPP

#include <jlm/tooling/CommandGraph.hpp>
#include <jlm/tooling/CommandLine.hpp>
#include <jlm/util/file.hpp>

#include <memory>
#include <string>

namespace jlm::llvm
{
class RvsdgModule;
}

namespace jlm::tooling
{

/** \brief Command class
 *
 * This class represents simple commands, such as \a mkdir or \a rm, that can be executed with the
 * Run() method.
 */
class Command
{
public:
  virtual ~Command();

  [[nodiscard]] virtual std::string
  ToString() const = 0;

  virtual void
  Run() const;
};

/**
 * The PrintCommandsCommand class prints the commands of a command graph in topological order.
 */
class PrintCommandsCommand final : public Command
{
public:
  ~PrintCommandsCommand() override;

  explicit PrintCommandsCommand(std::unique_ptr<CommandGraph> commandGraph)
      : CommandGraph_(std::move(commandGraph))
  {}

  PrintCommandsCommand(const PrintCommandsCommand &) = delete;

  PrintCommandsCommand(PrintCommandsCommand &&) = delete;

  PrintCommandsCommand &
  operator=(const PrintCommandsCommand &) = delete;

  PrintCommandsCommand &
  operator=(PrintCommandsCommand &&) = delete;

  [[nodiscard]] std::string
  ToString() const override;

  void
  Run() const override;

  static std::unique_ptr<CommandGraph>
  Create(std::unique_ptr<CommandGraph> commandGraph);

private:
  static CommandGraph::Node &
  Create(CommandGraph & commandGraph, std::unique_ptr<CommandGraph> printedCommandGraph)
  {
    auto command = std::make_unique<PrintCommandsCommand>(std::move(printedCommandGraph));
    return CommandGraph::Node::Create(commandGraph, std::move(command));
  }

  std::unique_ptr<CommandGraph> CommandGraph_;
};

/**
 * The ClangCommand class represents the clang command line tool.
 */
class ClangCommand final : public Command
{
public:
  enum class LanguageStandard
  {
    Unspecified,
    Gnu89,
    Gnu99,
    C89,
    C99,
    C11,
    Cpp98,
    Cpp03,
    Cpp11,
    Cpp14
  };

  enum class ClangArgument
  {
    DisableO0OptNone
  };

  ~ClangCommand() override;

  ClangCommand(
      std::vector<util::FilePath> inputFiles,
      util::FilePath outputFile,
      std::vector<std::string> libraryPaths,
      std::vector<std::string> libraries,
      bool usePthreads)
      : InputFiles_(std::move(inputFiles)),
        OutputFile_(std::move(outputFile)),
        DependencyFile_(""),
        Libraries_(std::move(libraries)),
        LibraryPaths_(std::move(libraryPaths)),
        UsePthreads_(usePthreads),
        Verbose_(false),
        Rdynamic_(false),
        Suppress_(false),
        Md_(false),
        LanguageStandard_(LanguageStandard::Unspecified),
        LinkerCommand_(true)
  {}

  ClangCommand(
      const util::FilePath & inputFile,
      util::FilePath outputFile,
      util::FilePath dependencyFile,
      std::vector<std::string> includePaths,
      std::vector<std::string> macroDefinitions,
      std::vector<std::string> warnings,
      std::vector<std::string> flags,
      bool verbose,
      bool rdynamic,
      bool suppress,
      bool usePthreads,
      bool mD,
      std::string mT,
      const LanguageStandard & languageStandard,
      std::vector<ClangArgument> clangArguments)
      : InputFiles_({ inputFile }),
        OutputFile_(std::move(outputFile)),
        DependencyFile_(std::move(dependencyFile)),
        IncludePaths_(std::move(includePaths)),
        MacroDefinitions_(std::move(macroDefinitions)),
        Warnings_(std::move(warnings)),
        Flags_(std::move(flags)),
        UsePthreads_(usePthreads),
        Verbose_(verbose),
        Rdynamic_(rdynamic),
        Suppress_(suppress),
        Md_(mD),
        Mt_(std::move(mT)),
        LanguageStandard_(languageStandard),
        ClangArguments_(std::move(clangArguments)),
        LinkerCommand_(false)
  {}

  [[nodiscard]] std::string
  ToString() const override;

  [[nodiscard]] const util::FilePath &
  OutputFile() const noexcept
  {
    return OutputFile_;
  }

  [[nodiscard]] const std::vector<util::FilePath> &
  InputFiles() const noexcept
  {
    return InputFiles_;
  }

  static CommandGraph::Node &
  CreateLinkerCommand(
      CommandGraph & commandGraph,
      const std::vector<util::FilePath> & inputFiles,
      const util::FilePath & outputFile,
      const std::vector<std::string> & libraryPaths,
      const std::vector<std::string> & libraries,
      bool usePthreads)
  {
    std::unique_ptr<ClangCommand> command(
        new ClangCommand(inputFiles, outputFile, libraryPaths, libraries, usePthreads));
    return CommandGraph::Node::Create(commandGraph, std::move(command));
  }

  static CommandGraph::Node &
  CreateParsingCommand(
      CommandGraph & commandGraph,
      const util::FilePath & inputFile,
      const util::FilePath & outputFile,
      const util::FilePath & dependencyFile,
      const std::vector<std::string> & includePaths,
      const std::vector<std::string> & macroDefinitions,
      const std::vector<std::string> & warnings,
      const std::vector<std::string> & flags,
      bool verbose,
      bool rdynamic,
      bool suppress,
      bool usePthread,
      bool mD,
      const std::string & mT,
      const LanguageStandard & languageStandard,
      const std::vector<ClangArgument> & clangArguments)
  {
    std::unique_ptr<ClangCommand> command(new ClangCommand(
        inputFile,
        outputFile,
        dependencyFile,
        includePaths,
        macroDefinitions,
        warnings,
        flags,
        verbose,
        rdynamic,
        suppress,
        usePthread,
        mD,
        mT,
        languageStandard,
        clangArguments));
    return CommandGraph::Node::Create(commandGraph, std::move(command));
  }

private:
  static std::string
  ToString(const LanguageStandard & languageStandard);

  static std::string
  ToString(const ClangArgument & clangArgument);

  static std::string
  ReplaceAll(std::string str, const std::string & from, const std::string & to);

  std::vector<util::FilePath> InputFiles_;
  util::FilePath OutputFile_;
  util::FilePath DependencyFile_;

  std::vector<std::string> IncludePaths_;
  std::vector<std::string> MacroDefinitions_;
  std::vector<std::string> Warnings_;
  std::vector<std::string> Flags_;
  std::vector<std::string> Libraries_;
  std::vector<std::string> LibraryPaths_;

  bool UsePthreads_;
  bool Verbose_;
  bool Rdynamic_;
  bool Suppress_;
  bool Md_;
  std::string Mt_;

  LanguageStandard LanguageStandard_;
  std::vector<ClangArgument> ClangArguments_;

  bool LinkerCommand_;
};

/**
 * The LlcCommand class represents the llc command line tool.
 */
class LlcCommand final : public Command
{
public:
  enum class OptimizationLevel
  {
    O0,
    O1,
    O2,
    O3
  };

  enum class RelocationModel
  {
    Static,
    Pic
  };

  ~LlcCommand() override;

  LlcCommand(
      util::FilePath inputFile,
      util::FilePath outputFile,
      const OptimizationLevel & optimizationLevel,
      const RelocationModel & relocationModel)
      : OptimizationLevel_(optimizationLevel),
        RelocationModel_(relocationModel),
        InputFile_(std::move(inputFile)),
        OutputFile_(std::move(outputFile))
  {}

  [[nodiscard]] std::string
  ToString() const override;

  [[nodiscard]] const util::FilePath &
  OutputFile() const noexcept
  {
    return OutputFile_;
  }

  static CommandGraph::Node &
  Create(
      CommandGraph & commandGraph,
      const util::FilePath & inputFile,
      const util::FilePath & outputFile,
      const OptimizationLevel & optimizationLevel,
      const RelocationModel & relocationModel)
  {
    std::unique_ptr<LlcCommand> command(
        new LlcCommand(inputFile, outputFile, optimizationLevel, relocationModel));
    return CommandGraph::Node::Create(commandGraph, std::move(command));
  }

private:
  static std::string
  ToString(const OptimizationLevel & optimizationLevel);

  static std::string
  ToString(const RelocationModel & relocationModel);

  OptimizationLevel OptimizationLevel_;
  RelocationModel RelocationModel_;
  util::FilePath InputFile_;
  util::FilePath OutputFile_;
};

/**
 * The JlmOptCommand class represents the jlm-opt command line tool.
 */
class JlmOptCommand final : public Command
{
public:
  ~JlmOptCommand() override;

  JlmOptCommand(std::string programName, const JlmOptCommandLineOptions & commandLineOptions);

  [[nodiscard]] std::string
  ToString() const override;

  void
  Run() const override;

  static CommandGraph::Node &
  Create(
      CommandGraph & commandGraph,
      std::string programName,
      const JlmOptCommandLineOptions & commandLineOptions)
  {
    auto command =
        std::make_unique<JlmOptCommand>(std::move(programName), std::move(commandLineOptions));
    return CommandGraph::Node::Create(commandGraph, std::move(command));
  }

  [[nodiscard]] const JlmOptCommandLineOptions &
  GetCommandLineOptions() const noexcept
  {
    return CommandLineOptions_;
  }

  static void
  PrintRvsdgModule(
      llvm::RvsdgModule & rvsdgModule,
      const util::FilePath & outputFile,
      const JlmOptCommandLineOptions::OutputFormat & outputFormat,
      util::StatisticsCollector & statisticsCollector);

private:
  std::unique_ptr<llvm::RvsdgModule>
  ParseInputFile(
      const util::FilePath & inputFile,
      const JlmOptCommandLineOptions::InputFormat & inputFormat,
      util::StatisticsCollector & statisticsCollector) const;

  std::unique_ptr<llvm::RvsdgModule>
  ParseLlvmIrFile(const util::FilePath & inputFile, util::StatisticsCollector & statisticsCollector)
      const;

  std::unique_ptr<llvm::RvsdgModule>
  ParseMlirIrFile(const util::FilePath & inputFile, util::StatisticsCollector & statisticsCollector)
      const;

  static void
  PrintAsAscii(
      const llvm::RvsdgModule & rvsdgModule,
      const util::FilePath & outputFile,
      util::StatisticsCollector & statisticsCollector);

  static void
  PrintAsXml(
      const llvm::RvsdgModule & rvsdgModule,
      const util::FilePath & outputFile,
      util::StatisticsCollector & statisticsCollector);

  static void
  PrintAsLlvm(
      llvm::RvsdgModule & rvsdgModule,
      const util::FilePath & outputFile,
      util::StatisticsCollector & statisticsCollector);

  static void
  PrintAsMlir(
      const llvm::RvsdgModule & rvsdgModule,
      const util::FilePath & outputFile,
      util::StatisticsCollector & statisticsCollector);

  static void
  PrintAsRvsdgTree(
      const llvm::RvsdgModule & rvsdgModule,
      const util::FilePath & outputFile,
      util::StatisticsCollector & statisticsCollector);

  static void
  PrintAsDot(
      const llvm::RvsdgModule & rvsdgModule,
      const util::FilePath & outputFile,
      util::StatisticsCollector & statisticsCollector);

  [[nodiscard]] std::vector<rvsdg::Transformation *>
  GetTransformations() const;

  [[nodiscard]] std::unique_ptr<rvsdg::Transformation>
  CreateTransformation(JlmOptCommandLineOptions::OptimizationId optimizationId) const;

  std::string ProgramName_;
  JlmOptCommandLineOptions CommandLineOptions_;
  std::unordered_map<
      JlmOptCommandLineOptions::OptimizationId,
      std::unique_ptr<rvsdg::Transformation>>
      Optimizations_ = {};
};

/**
 * The MkdirCommand class represents the mkdir command line tool.
 */
class MkdirCommand final : public Command
{
public:
  ~MkdirCommand() noexcept override;

  explicit MkdirCommand(util::FilePath path)
      : Path_(std::move(path))
  {}

  [[nodiscard]] std::string
  ToString() const override;

  void
  Run() const override;

  static CommandGraph::Node &
  Create(CommandGraph & commandGraph, const util::FilePath & path)
  {
    std::unique_ptr<MkdirCommand> command(new MkdirCommand(path));
    return CommandGraph::Node::Create(commandGraph, std::move(command));
  }

private:
  util::FilePath Path_;
};

/**
 * The LlvmOptCommand class represents the LLVM opt command line tool.
 */
class LlvmOptCommand final : public Command
{
public:
  enum class Optimization
  {
    Mem2Reg,
  };

  ~LlvmOptCommand() noexcept override;

  LlvmOptCommand(
      util::FilePath inputFile,
      util::FilePath outputFile,
      bool writeLlvmAssembly,
      std::vector<Optimization> optimizations)
      : InputFile_(std::move(inputFile)),
        OutputFile_(std::move(outputFile)),
        WriteLlvmAssembly_(writeLlvmAssembly),
        Optimizations_(std::move(optimizations))
  {}

  [[nodiscard]] std::string
  ToString() const override;

  [[nodiscard]] const util::FilePath &
  OutputFile() const noexcept
  {
    return OutputFile_;
  }

  static CommandGraph::Node &
  Create(
      CommandGraph & commandGraph,
      const util::FilePath & inputFile,
      const util::FilePath & outputFile,
      bool writeLlvmAssembly,
      const std::vector<Optimization> & optimizations)
  {
    std::unique_ptr<LlvmOptCommand> command(
        new LlvmOptCommand(inputFile, outputFile, writeLlvmAssembly, optimizations));
    return CommandGraph::Node::Create(commandGraph, std::move(command));
  }

private:
  static std::string
  ToString(const Optimization & optimization);

  util::FilePath InputFile_;
  util::FilePath OutputFile_;

  bool WriteLlvmAssembly_;

  std::vector<Optimization> Optimizations_;
};

/**
 * The LlvmLinkCommand class represents the llvm-link command line tool.
 */
class LlvmLinkCommand final : public Command
{
public:
  ~LlvmLinkCommand() noexcept override;

  LlvmLinkCommand(
      std::vector<util::FilePath> inputFiles,
      util::FilePath outputFile,
      bool writeLlvmAssembly,
      bool verbose)
      : OutputFile_(std::move(outputFile)),
        InputFiles_(std::move(inputFiles)),
        WriteLlvmAssembly_(writeLlvmAssembly),
        Verbose_(verbose)
  {}

  [[nodiscard]] std::string
  ToString() const override;

  [[nodiscard]] const util::FilePath &
  OutputFile() const noexcept
  {
    return OutputFile_;
  }

  [[nodiscard]] const std::vector<util::FilePath> &
  InputFiles() const noexcept
  {
    return InputFiles_;
  }

  static CommandGraph::Node &
  Create(
      CommandGraph & commandGraph,
      const std::vector<util::FilePath> & inputFiles,
      const util::FilePath & outputFile,
      bool writeLlvmAssembly,
      bool verbose)
  {
    std::unique_ptr<LlvmLinkCommand> command(
        new LlvmLinkCommand(inputFiles, outputFile, writeLlvmAssembly, verbose));
    return CommandGraph::Node::Create(commandGraph, std::move(command));
  }

private:
  util::FilePath OutputFile_;
  std::vector<util::FilePath> InputFiles_;

  bool WriteLlvmAssembly_;
  bool Verbose_;
};

/**
 * The JlmHlsCommand class represents the jlm-hls command line tool.
 */
class JlmHlsCommand final : public Command
{
public:
  ~JlmHlsCommand() noexcept override;

  JlmHlsCommand(
      util::FilePath inputFile,
      util::FilePath outputFolder,
      std::vector<std::string> options)
      : InputFile_(std::move(inputFile)),
        OutputFolder_(std::move(outputFolder)),
        Options(std::move(options))
  {}

  [[nodiscard]] std::string
  ToString() const override;

  [[nodiscard]] util::FilePath
  FirrtlFile() const noexcept
  {
    return OutputFolder_.WithSuffix(".fir");
  }

  [[nodiscard]] util::FilePath
  LlvmFile() const noexcept
  {
    return OutputFolder_.WithSuffix(".rest.ll");
  }

  [[nodiscard]] util::FilePath
  RefFile() const noexcept
  {
    return OutputFolder_.WithSuffix(".ref.ll");
  }

  [[nodiscard]] util::FilePath
  HarnessFile() const noexcept
  {
    return OutputFolder_.WithSuffix(".harness.cpp");
  }

  [[nodiscard]] const util::FilePath &
  InputFile() const noexcept
  {
    return InputFile_;
  }

  static CommandGraph::Node &
  Create(
      CommandGraph & commandGraph,
      const util::FilePath & inputFile,
      const util::FilePath & outputFolder,
      const std::vector<std::string> & options)
  {
    auto command = std::make_unique<JlmHlsCommand>(inputFile, outputFolder, options);
    return CommandGraph::Node::Create(commandGraph, std::move(command));
  }

private:
  util::FilePath InputFile_;
  util::FilePath OutputFolder_;
  std::vector<std::string> Options;
};

/**
 * The JlmHlsExtractCommand class represents the jlm-hls command line tool with the --extract
 * command line argument provided.
 */
class JlmHlsExtractCommand final : public Command
{
public:
  ~JlmHlsExtractCommand() noexcept override;

  JlmHlsExtractCommand(
      util::FilePath inputFile,
      util::FilePath outputFolder,
      std::string hlsFunctionName)
      : InputFile_(std::move(inputFile)),
        OutputFolder_(std::move(outputFolder)),
        HlsFunctionName_(std::move(hlsFunctionName))
  {}

  [[nodiscard]] std::string
  ToString() const override;

  [[nodiscard]] util::FilePath
  HlsFunctionFile() const noexcept
  {
    return OutputFolder_.WithSuffix(".function.ll");
  }

  [[nodiscard]] util::FilePath
  LlvmFile() const noexcept
  {
    return OutputFolder_.WithSuffix(".rest.ll");
  }

  [[nodiscard]] const util::FilePath &
  InputFile() const noexcept
  {
    return InputFile_;
  }

  [[nodiscard]] const std::string &
  HlsFunctionName() const noexcept
  {
    return HlsFunctionName_;
  }

  static CommandGraph::Node &
  Create(
      CommandGraph & commandGraph,
      const util::FilePath & inputFile,
      const std::string & hlsFunctionName,
      const util::FilePath & outputFolder)
  {
    std::unique_ptr<JlmHlsExtractCommand> command(
        new JlmHlsExtractCommand(inputFile, outputFolder, hlsFunctionName));
    return CommandGraph::Node::Create(commandGraph, std::move(command));
  }

private:
  util::FilePath InputFile_;
  util::FilePath OutputFolder_;

  std::string HlsFunctionName_;
};

}

#endif
