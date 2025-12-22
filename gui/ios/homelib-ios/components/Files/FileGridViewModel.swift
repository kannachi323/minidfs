//
//  FileGridViewModel.swift
//  homelib-ios
//
//  Created by Matthew Chen on 7/21/25.
//

import Foundation
import Combine
import SwiftUICore

struct FileItem: Identifiable, Decodable {
    let fileId: String
    let name: String
    let path: String
    let size: UInt128
    let isDir: Bool
    
    var id: String { fileId }
}

enum FilterTag : String, CaseIterable {
    case sortByName = "Name"
    case sortBySize = "Size"
}


class FileGridViewModel: ObservableObject {
    @Published var files: [FileItem] = []
    private var ogFiles : [FileItem] = []
    
    @Published var showError = false
    @Published var errorMessage: String?
    
    @Published var currentPath: String = ""
    
    @Published var filterTag: FilterTag = .sortByName

    func fetchFiles(path: String) {
        guard let url = URL(string: "http://localhost:8080/files?path=/\(path)") else {
            self.triggerError(errorMessage: "Invalid URL")

            return
        }
        
        URLSession.shared.dataTask(with: url) { data, response, error in
            if let error = error {
                DispatchQueue.main.async {
                    self.triggerError(errorMessage: "Error: \(error.localizedDescription)")
                }
                return
            }
            
            guard let data = data else {
                DispatchQueue.main.async {
                    self.triggerError(errorMessage: "No data received")
                }
                return
            }
            
            do {
                let decodedFiles = try JSONDecoder().decode([FileItem].self, from: data)
                DispatchQueue.main.async {
                    self.files = decodedFiles
                    self.ogFiles = decodedFiles
                }
            } catch {
                DispatchQueue.main.async {
                    self.triggerError(errorMessage: "Decoding error: \(error.localizedDescription)")
                }
            }
        }.resume()
    }
    
    func goBack() -> String {
        let path = URL(fileURLWithPath: self.currentPath).deletingLastPathComponent().path
        currentPath = path
        return currentPath
    }
    
    func getDirectory() -> String? {
        let path = URL(fileURLWithPath: self.currentPath).lastPathComponent
        if (path == "/") {
            return nil
        }
        
        return URL(fileURLWithPath: self.currentPath).lastPathComponent
    }
    
    func sortFilesByName(inc: Bool) {
        filterTag = .sortByName
        if inc {
            files.sort {
                $0.name.localizedCompare($1.name) == .orderedAscending
            }
        } else {
            files.sort {
                $0.name.localizedCompare($1.name) == .orderedDescending
            }
        }
        
    }
    
    func sortFilesBySize(inc: Bool) {
        filterTag = .sortBySize
        if (inc) {
            files.sort {
                $0.size < $1.size
            }
        } else {
            files.sort {
                $0.size > $1.size
            }
        }
        
    }
    
    func filterFoldersOnly() {
        files = files.filter {
            $0.isDir == true
        }
    }
    
    func updateFilters(inc: Bool) {
        
        print(self.filterTag)
        
        switch self.filterTag {
            case .sortByName:
                self.sortFilesByName(inc: inc)
            case .sortBySize:
                self.sortFilesBySize(inc: inc)
        }
    }
 
    func clearFilters() {
        files = ogFiles
    }
    
    func triggerError(errorMessage : String) {
        self.errorMessage = errorMessage
        showError.toggle()
    }
}

class MockFileGridViewModel: FileGridViewModel {
    override func fetchFiles(path: String) {
        self.files = [
            FileItem(fileId: "1", name: "Test Folder", path: "Test", size: 0, isDir: true),
            FileItem(fileId: "2", name: "Test.txt", path: "Test.txt", size: 512, isDir: false),
            FileItem(fileId: "3", name: "Test Folder2", path: "Test", size: 0, isDir: true),
            FileItem(fileId: "4", name: "Test2.txt", path: "Test.txt", size: 512, isDir: false)
        ]
    }
}
