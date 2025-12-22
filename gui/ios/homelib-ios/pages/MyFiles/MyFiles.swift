//
//  MyFiles.swift
//  homelib-ios
//
//  Created by Matthew Chen on 7/20/25.
//

import SwiftUI

struct MyFilesView : View {
    @StateObject var fileGridViewModel = FileGridViewModel()
    @State var searchText = ""
    @State var isOpen = false
    
    var body : some View {
        ZStack(alignment: .leading) {
            VStack(alignment: .leading) {
                MyFilesNavView(viewModel: fileGridViewModel)
                    
 
                SearchBarView(searchText: $searchText, isOpen: $isOpen)

                  
                MyFilesFilterView(viewModel: fileGridViewModel)
                
                
                FileGridView(viewModel: fileGridViewModel)
                  
            }
            .padding()
        }
        .onAppear {
            fileGridViewModel.fetchFiles(path: fileGridViewModel.currentPath)
        }
    }
}

struct MyFilesFilterView: View {
    @ObservedObject var viewModel: FileGridViewModel
    @State var inc : Bool = true
    
    var body: some View {
        HStack() {
            HStack(spacing: 10) {
                Menu {
                    Button("Sort by Name") {
                        viewModel.sortFilesByName(inc: inc)
                    }
                    Button("Sort by Size") {
                           viewModel.sortFilesBySize(inc: inc)
                    }

                    Button("Clear Filters") {
                        viewModel.clearFilters()
                    }
                    
                } label: {
                    Text(viewModel.filterTag.rawValue)
                        .font(.headline)
                  
                }
                .foregroundColor(.white)
                
                Image(systemName: inc ? "arrowtriangle.up" : "arrowtriangle.down")
                    .font(.title2)
                    .imageScale(.small)
                    .onTapGesture {
                        inc.toggle()
                        viewModel.updateFilters(inc: inc)
                    }
                    
            }
            
            Spacer()
            
            
            Menu {
                Section(header: Text("File Type")) {
                    Button("Folders") {
                        viewModel.filterFoldersOnly()
                    }
                    Button("Photos (.jpg, .png, etc.)") {
                        viewModel.sortFilesByName(inc: true)
                    }
                    Button("Videos (.mp4, .mov, etc.)") {
                        viewModel.sortFilesBySize(inc: true)
                    }
                    
                    Button("Other (custom types)") {
                        viewModel.sortFilesByName(inc: true)
                    }
                }
                
            } label: {
                Image(systemName: "line.3.horizontal.decrease.circle")
                
              
            }
            .foregroundColor(.white)
            .font(.title2)
            .imageScale(.medium)
        }
        .padding()
       
    }
}


struct MyFilesNavView: View {
    @ObservedObject var viewModel: FileGridViewModel
    
    var body: some View {
        
        ZStack {
            HStack {
                if let dir = viewModel.getDirectory() {
                    Image(systemName: "chevron.left")
                        .font(.title)
                        .bold()
                        .onTapGesture {
                            viewModel.fetchFiles(path: viewModel.goBack())
                        }
                    Text(dir)
                        .font(.title)
                        .bold()
                        .lineLimit(1)
                        .truncationMode(.tail)
                } else {
                    Text("Files")
                        .font(.largeTitle)
                        .bold()
                        .lineLimit(1)
                        .truncationMode(.tail)
                }
                
                Spacer()
                
                // Right side icons
                HStack(spacing: 15) {
                    Image(systemName: "arrow.up.document")
                        .font(.title2)
                        .imageScale(.large)
                    
                    Image(systemName: "checkmark.square")
                        .font(.title2)
                        .imageScale(.large)
                    
                    Image(systemName: "ellipsis.circle")
                        .font(.title2)
                        .imageScale(.large)
                }
            }
            .padding(.horizontal)
            .frame(height: 45)
        }
    }
}

#Preview {
    return MyFilesView()
}
