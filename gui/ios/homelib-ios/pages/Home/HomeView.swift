//
//  HomeView.swift
//  homelib-ios
//
//  Created by Matthew Chen on 7/20/25.
//

import SwiftUI
import Foundation
import Combine

struct HomeView: View {
    @State private var searchText : String = ""
    @State private var isDrawerOpen: Bool = false
    @StateObject private var viewModel = FileGridViewModel()
    

    var body: some View {
        ZStack(alignment: .leading) {
            VStack(alignment: .leading, spacing: 16) {
                Text("HomeLib")
                    .font(.title)
                    .bold()
                
                SearchBarView(searchText: $searchText, isOpen: $isDrawerOpen)
                
                HomeDropdownView()
                
                FileGridView(viewModel: viewModel)
            }
            .padding()
            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
            .alert(isPresented: $viewModel.showError) {
                Alert(
                    title: Text("Something went wrong"),
                    message: Text(viewModel.errorMessage ?? "Unknown error"),
                    dismissButton: .default(Text("OK"))
                )
            }
            .onAppear {
                viewModel.fetchFiles(path: "/")
            }
            .zIndex(0)
            
            VStack(alignment: .leading) {
                if (isDrawerOpen) {
                    DrawerView(isOpen: $isDrawerOpen)
                }
            }
            .frame(maxWidth: 250)
            .zIndex(10)
        }
       
    }
}



struct HomePreview : View {
    @State var text = ""
    var body: some View {
        HomeView()
    }
    
}

#Preview {
    HomePreview()
}
