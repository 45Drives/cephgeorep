#pragma once

#include <vector>
#include <string>

class Syncer;
class File;

class SyncProcess{
private:
	int id_;
	/* Integral ID for each process to be used while printing
	 * log messages.
	 */
	int inc_;
	/* Amount to increment the file list iterator by.
	 */
	pid_t pid_;
	/* PID of process.
	 */
	size_t start_payload_sz_;
	/* Count of elements in argv for sync process without any files added
	 */
	size_t max_mem_usage_;
	/* Maximum number of bytes allowed in environment and argv of called
	 * process.
	 */
	size_t start_mem_usage_;
	/* Number of bytes contained in the environment and in the
	 * executable path and flags.
	 */
	size_t curr_mem_usage_;
	/* Keeps track of bytes in argument vector.
	 */
	uintmax_t curr_payload_bytes_;
	/* Keeps track of payload size.
	 */
	std::string exec_bin_;
	/* File syncing program name.
	 */
	std::string exec_flags_;
	/* Flags and extra args for program.
	 */
	std::vector<std::string>::iterator &destination_;
	/* [[<user>@]<host>:][<destination path>]
	 */
	std::string sending_to_;
	/* For printing failures after iterator changes.
	 */
	std::vector<File>::iterator file_itr_;
	/* Iterator to list of files to sync.
	 */
	std::vector<char *> payload_;
	/* argv for sync process
	 */
public:
	SyncProcess(Syncer *parent, int id, int nproc, std::vector<File> &queue);
	/* Constructor. Grabs members from parent pointer.
	 */
	~SyncProcess();
	/* Destructor. Frees memory in garbage vector before returning.
	 */
	int id() const;
	/* Return ID of sync process.
	 */
	pid_t pid() const;
	/* Return PID of sync process.
	 */
	void set_id(int id);
	/* Assign integral ID.
	 */
	uintmax_t payload_sz(void) const;
	/* Return number of bytes in file payload.
	 */
	uintmax_t payload_count(void) const;
	/* Return number of files in payload.
	 */
	void add(const std::vector<File>::iterator &itr);
	/* Add one file to the payload.
	 * Incrememnts curr_arg_sz_ and curr_bytes_sz_ accordingly.
	 */
	bool full_test(const File &file) const;
	/* Returns true if curr_mem_usage_ exceeds the maximum
	 * if file were to be added.
	 */
	void consume(std::vector<File> &queue);
	/* Pop files from queue and push into files_ vector until full.
	 */
	void sync_batch(void);
	/* Fork and execute sync program with file batch.
	 */
	void reset(void);
	/* clear payload from start_payload_sz_ to end
	 * set curr_mem_usage_ to start_mem_usage_
	 * set curr_payload_bytes_ to 0
	 */
	bool done(const std::vector<File> &queue) const;
	/* Returns queue.end() - file_itr_ <= nrpoc.
	 */
	const std::string &destination(void) const;
	/* Returns sending_to_.
	 */
};
